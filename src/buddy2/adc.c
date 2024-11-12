#include "adc.h"
#include <stdio.h>
#include <stdlib.h>

// Private ADC configuration structure
typedef struct {
    uint16_t* capture_buf;
    uint capture_depth;
    uint button_pin;
    uint analog_pin;
    bool capturing;
    bool transfer_complete;
    float last_frequency;
    int dma_chan;
    bool continuous_mode;
} ADC_Config;

// Private global state
static ADC_Config adc_config = {
    .capture_buf = NULL,
    .capture_depth = DEFAULT_CAPTURE_DEPTH,
    .button_pin = ADC_BUTTON_PIN,
    .analog_pin = DEFAULT_ANALOG_PIN,
    .capturing = false,
    .transfer_complete = false,
    .last_frequency = 0.0f,
    .dma_chan = -1,
    .continuous_mode = false
};

static void dma_handler(void) {
    if (adc_config.dma_chan < 0) return;
    
    dma_hw->ints0 = 1u << adc_config.dma_chan;
    
    if (adc_config.continuous_mode) {
        adc_config.transfer_complete = true;
        
        dma_channel_config cfg = dma_channel_get_default_config(adc_config.dma_chan);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
        channel_config_set_read_increment(&cfg, false);
        channel_config_set_write_increment(&cfg, true);
        channel_config_set_dreq(&cfg, DREQ_ADC);
        
        dma_channel_configure(
            adc_config.dma_chan,
            &cfg,
            adc_config.capture_buf,
            &adc_hw->fifo,
            adc_config.capture_depth,
            true
        );
    } else {
        adc_run(false);
        adc_config.capturing = false;
        adc_config.transfer_complete = true;
    }    
}

void adc_analyzer_init(void) {
    printf("Initializing ADC and DMA...\n");
    
    // Allocate capture buffer
    adc_config.capture_buf = (uint16_t*)malloc(adc_config.capture_depth * sizeof(uint16_t));
    if (!adc_config.capture_buf) {
        printf("Error: Failed to allocate capture buffer\n");
        return;
    }
    
    // Initialize ADC
    adc_gpio_init(adc_config.analog_pin);
    adc_init();
    adc_select_input(0);
    
    adc_fifo_setup(
        true,    // Write each conversion to FIFO
        true,    // Enable DMA requests
        1,       // DREQ when at least 1 sample present
        false,   // Disable error bit
        false    // Don't shift samples
    );
    
    adc_set_clkdiv(4800);  // 10kHz sampling
    adc_fifo_drain();
    
    // Claim DMA channel
    adc_config.dma_chan = dma_claim_unused_channel(true);
    if (adc_config.dma_chan < 0) {
        printf("Error: Could not claim a DMA channel\n");
        return;
    }
    
    // Configure DMA interrupts
    dma_channel_set_irq0_enabled(adc_config.dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
}

void adc_start_capture(void) {
    if (!adc_config.capturing) {
        printf("\nStarting continuous capture...\n");
        adc_config.capturing = true;
        adc_config.transfer_complete = false;
        adc_config.continuous_mode = true;
        
        dma_channel_config cfg = dma_channel_get_default_config(adc_config.dma_chan);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
        channel_config_set_read_increment(&cfg, false);
        channel_config_set_write_increment(&cfg, true);
        channel_config_set_dreq(&cfg, DREQ_ADC);
        
        dma_channel_configure(
            adc_config.dma_chan,
            &cfg,
            adc_config.capture_buf,
            &adc_hw->fifo,
            adc_config.capture_depth,
            true
        );
        
        adc_run(true);
    }
}

void adc_stop_capture(void) {
    if (adc_config.capturing) {
        printf("Stopping capture...\n");
        adc_config.continuous_mode = false;
        adc_config.capturing = false;
        adc_run(false);
        dma_channel_abort(adc_config.dma_chan);
        adc_fifo_drain();
        adc_config.transfer_complete = true;
    }
}

bool is_adc_capturing(void) {
    return adc_config.capturing;
}

float get_last_frequency(void) {
    return adc_config.last_frequency;
}

bool is_transfer_complete(void) {
    return adc_config.transfer_complete;
}

void clear_transfer_complete(void) {
    adc_config.transfer_complete = false;
}

float analyze_current_capture(void) {
    uint16_t max_val = 0;
    uint16_t min_val = 4096;
    
    for(int i = 0; i < adc_config.capture_depth; i++) {
        if(adc_config.capture_buf[i] > max_val) max_val = adc_config.capture_buf[i];
        if(adc_config.capture_buf[i] < min_val && adc_config.capture_buf[i] != 0) min_val = adc_config.capture_buf[i];
    }
    
    uint16_t amplitude = max_val - min_val;
    float frequency = 0.0f;
    
    if(amplitude > 500) {
        uint16_t threshold = (max_val + min_val) / 2;
        uint16_t hysteresis = amplitude / 10;
        uint16_t upper_threshold = threshold + hysteresis;
        uint16_t lower_threshold = threshold - hysteresis;
        uint32_t first_crossing = 0;
        uint32_t last_crossing = 0;
        int crossing_count = 0;
        bool above = adc_config.capture_buf[0] > threshold;
        
        for(int i = 1; i < adc_config.capture_depth; i++) {
            if(adc_config.capture_buf[i] != 0) {
                if(above && adc_config.capture_buf[i] < lower_threshold) {
                    if(first_crossing == 0) first_crossing = i;
                    last_crossing = i;
                    crossing_count++;
                    above = false;
                }
                else if(!above && adc_config.capture_buf[i] > upper_threshold) {
                    if(first_crossing == 0) first_crossing = i;
                    last_crossing = i;
                    crossing_count++;
                    above = true;
                }
            }
        }
        
        if(crossing_count >= 4) {
            float sample_period = 1.0f / 10000.0f;  // 10kHz sampling
            float measurement_time = (last_crossing - first_crossing) * sample_period;
            float cycles = (float)(crossing_count) / 2.0f;
            frequency = cycles / measurement_time;
            
            if(frequency > 1.0f && frequency < 5000.0f) {
                adc_config.last_frequency = frequency;
            } else {
                printf("  Frequency out of range (1-5000 Hz)\n");
            }
        } else {
            printf("  Not enough signal transitions for frequency measurement\n");
        }
    } else {
        printf("  Signal amplitude too low for analysis\n");
    }
    
    if (adc_config.continuous_mode) {
        printf("\nContinuing capture...\n");
    } else {
        printf("\nPress again to capture\n");
    }
    return frequency;
}

void adc_cleanup(void) {
    if (adc_config.dma_chan >= 0) {
        dma_channel_unclaim(adc_config.dma_chan);
    }
    if (adc_config.capture_buf) {
        free(adc_config.capture_buf);
        adc_config.capture_buf = NULL;
    }
}