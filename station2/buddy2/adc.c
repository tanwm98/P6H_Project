#include "adc.h"
#include <stdio.h>
#include <stdlib.h>

// Global pointer to current config for use in callbacks
static ADC_Config* current_config = NULL;

static void dma_handler(void) {
    if (!current_config || current_config->dma_chan < 0) return;
    
    dma_hw->ints0 = 1u << current_config->dma_chan;
    
    // In continuous mode, immediately restart capture after analysis
    if (current_config->continuous_mode) {
        current_config->transfer_complete = true;
        
        // Reconfigure DMA for next capture
        dma_channel_config cfg = dma_channel_get_default_config(current_config->dma_chan);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
        channel_config_set_read_increment(&cfg, false);
        channel_config_set_write_increment(&cfg, true);
        channel_config_set_dreq(&cfg, DREQ_ADC);
        
        dma_channel_configure(
            current_config->dma_chan,
            &cfg,
            current_config->capture_buf,
            &adc_hw->fifo,
            current_config->capture_depth,
            true
        );
    } else {
        adc_run(false);
        current_config->capturing = false;
        current_config->transfer_complete = true;
    }
    
    printf("DMA Transfer Complete\n");
}

void adc_init_config(ADC_Config* config) {
    config->capture_depth = DEFAULT_CAPTURE_DEPTH;
    config->button_pin = DEFAULT_BUTTON_PIN;
    config->analog_pin = DEFAULT_ANALOG_PIN;
    config->capturing = false;
    config->transfer_complete = false;
    config->last_frequency = 0.0f;
    config->dma_chan = -1;
    config->continuous_mode = false;  // Initialize continuous mode as false
    
    // Allocate capture buffer
    config->capture_buf = (uint16_t*)malloc(config->capture_depth * sizeof(uint16_t));
    if (!config->capture_buf) {
        printf("Error: Failed to allocate capture buffer\n");
        return;
    }
}

bool adc_init_hardware(ADC_Config* config) {
    if (!config || !config->capture_buf) return false;
    
    current_config = config;  // Set global pointer for callbacks
    
    printf("Initializing ADC and DMA...\n");
    
    // Initialize button GPIO
    gpio_init(config->button_pin);
    gpio_set_dir(config->button_pin, GPIO_IN);
    gpio_pull_up(config->button_pin);
    gpio_set_irq_enabled_with_callback(config->button_pin, GPIO_IRQ_EDGE_FALL, true, &adc_button_handler);
    
    // Initialize ADC
    adc_gpio_init(config->analog_pin);
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
    config->dma_chan = dma_claim_unused_channel(true);
    if (config->dma_chan < 0) {
        printf("Error: Could not claim a DMA channel\n");
        return false;
    }
    
    // Configure DMA interrupts
    dma_channel_set_irq0_enabled(config->dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    
    printf("ADC and DMA initialization complete\n");
    return true;
}

void adc_start_capture(ADC_Config* config) {
    if (!config->capturing) {
        printf("\nStarting continuous capture...\n");
        config->capturing = true;
        config->transfer_complete = false;
        config->continuous_mode = true;  // Enable continuous mode
        
        // Configure DMA transfer
        dma_channel_config cfg = dma_channel_get_default_config(config->dma_chan);
        channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
        channel_config_set_read_increment(&cfg, false);
        channel_config_set_write_increment(&cfg, true);
        channel_config_set_dreq(&cfg, DREQ_ADC);
        
        dma_channel_configure(
            config->dma_chan,
            &cfg,
            config->capture_buf,
            &adc_hw->fifo,
            config->capture_depth,
            true
        );
        
        adc_run(true);
    }
}

void adc_stop_capture(ADC_Config* config) {
    if (config->capturing) {
        printf("Stopping capture...\n");
        config->continuous_mode = false;  // Disable continuous mode
        config->capturing = false;
        adc_run(false);
        dma_channel_abort(config->dma_chan);
        adc_fifo_drain();
        config->transfer_complete = true;
    }
}

void adc_button_handler(uint gpio, uint32_t events) {
    static uint32_t last_time = 0;
    uint32_t current_time = time_us_32();
    
    // Debounce - ignore events less than 200ms apart
    if (current_time - last_time < 200000) {
        return;
    }
    last_time = current_time;
    
    if (current_config && gpio == current_config->button_pin && (events & GPIO_IRQ_EDGE_FALL)) {
        if (!current_config->capturing) {
            adc_start_capture(current_config);
        } else {
            adc_stop_capture(current_config);
        }
    }
}

float adc_analyze_capture(ADC_Config* config) {
    uint16_t max_val = 0;
    uint16_t min_val = 4096;
    
    for(int i = 0; i < config->capture_depth; i++) {
        if(config->capture_buf[i] > max_val) max_val = config->capture_buf[i];
        if(config->capture_buf[i] < min_val && config->capture_buf[i] != 0) min_val = config->capture_buf[i];
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
        bool above = config->capture_buf[0] > threshold;
        
        for(int i = 1; i < config->capture_depth; i++) {
            if(config->capture_buf[i] != 0) {
                if(above && config->capture_buf[i] < lower_threshold) {
                    if(first_crossing == 0) first_crossing = i;
                    last_crossing = i;
                    crossing_count++;
                    above = false;
                }
                else if(!above && config->capture_buf[i] > upper_threshold) {
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
                printf("  Frequency: %.1f Hz\n", frequency);
                config->last_frequency = frequency;
            } else {
                printf("  Frequency out of range (1-5000 Hz)\n");
            }
        } else {
            printf("  Not enough signal transitions for frequency measurement\n");
        }
    } else {
        printf("  Signal amplitude too low for analysis\n");
    }
    
    if (config->continuous_mode) {
        printf("\nContinuing capture...\n");
    } else {
        printf("\nPress again to capture\n");
    }
    return frequency;
}

void adc_cleanup(ADC_Config* config) {
    if (config) {
        if (config->dma_chan >= 0) {
            dma_channel_unclaim(config->dma_chan);
        }
        if (config->capture_buf) {
            free(config->capture_buf);
        }
        current_config = NULL;
    }
}