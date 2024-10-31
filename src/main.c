#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include "hardware/dma.h"
#include "hardware/timer.h"
#include <stdio.h>
#include <string.h>

#define DIGITAL_PIN 2      // GP2 for digital pulses
#define PWM_PIN 7         // GP7 for PWM analysis
#define ANALOG_PIN 26     // GP26 (ADC0) for analog signals
#define ADC_CLOCK_DIV 0   // Run ADC at full speed for better frequency detection
#define ADC_RANGE 4096  // 12-bit ADC
#define MIN_AMPLITUDE 100  // Minimum peak-to-peak amplitude to consider valid signal
#define CAPTURE_DEPTH 5000          // Increased for longer capture window
#define MIN_FREQ 0.5f              // Minimum frequency to detect
#define MAX_FREQ 300.0f            // Maximum frequency to detect with margin
#define SAMPLE_RATE 1000.0f        // 2kHz sampling rate for good resolution
uint16_t capture_buf[CAPTURE_DEPTH];
uint dma_chan;

// Digital signal state
typedef struct {
    uint32_t last_rise_time;
    uint32_t last_fall_time;
    uint32_t pulse_width;
    bool current_state;
} DigitalSignal;
volatile DigitalSignal digital_signal = {0};

// PWM measurements
typedef struct {
    float frequency;
    float duty_cycle;
    uint32_t last_rise;
    uint32_t last_fall;
    uint32_t period;
} PWMMetrics;
volatile PWMMetrics pwm_metrics = {0};

// Analog frequency measurement
volatile float analog_frequency = 0.0f;

// GPIO interrupt handler
void gpio_callback(uint gpio, uint32_t events) {
    uint32_t now = time_us_32();
    if(gpio == DIGITAL_PIN) {
        if(events & GPIO_IRQ_EDGE_RISE) {
            digital_signal.last_rise_time = now;
            digital_signal.current_state = true;
            printf("Digital Signal: HIGH at %lu us\n", now);
        
            // Calculate previous LOW period if we have a valid last fall time
            if(digital_signal.last_fall_time != 0) {
                uint32_t low_width = now - digital_signal.last_fall_time;
                printf("LOW pulse width: %lu us\n", low_width);
            }
        }
        else if(events & GPIO_IRQ_EDGE_FALL) {
            digital_signal.last_fall_time = now;
            digital_signal.current_state = false;

            // Calculate HIGH pulse width
            if(digital_signal.last_rise_time != 0) {
                digital_signal.pulse_width = now - digital_signal.last_rise_time;
                printf("Digital Signal: LOW at %lu us\n", now);
                printf("HIGH pulse width: %lu us\n", digital_signal.pulse_width);
            }
        }
    }
    else if(gpio == PWM_PIN) {
        // Calculate both frequency and duty cycle for PWM
        if(events & GPIO_IRQ_EDGE_RISE) {
            if(pwm_metrics.last_rise != 0) {
                pwm_metrics.period = now - pwm_metrics.last_rise;
                pwm_metrics.frequency = 1000000.0f / pwm_metrics.period;
            }
            pwm_metrics.last_rise = now;
        } else if(events & GPIO_IRQ_EDGE_FALL) {
            if(pwm_metrics.last_rise != 0) {
                uint32_t high_time = now - pwm_metrics.last_rise;
                if(pwm_metrics.period > 0) {
                    pwm_metrics.duty_cycle = (float)high_time * 100.0f / pwm_metrics.period;
                }
            }
            pwm_metrics.last_fall = now;
            // Print PWM measurements after each complete cycle
            printf("PWM - Frequency: %.2f Hz, Duty Cycle: %.1f%%\n", 
                   pwm_metrics.frequency, pwm_metrics.duty_cycle);
        }
    }
}

float analyze_capture() {
    uint16_t max_val = 0;
    uint16_t min_val = 4096;
    // First pass: find valid min/max values
    for(int i = 0; i < CAPTURE_DEPTH; i++) {
        if(capture_buf[i] > max_val) max_val = capture_buf[i];
        if(capture_buf[i] < min_val && capture_buf[i] != 0) min_val = capture_buf[i];
    }
    uint16_t amplitude = max_val - min_val;
    float frequency = 0.0f;
    // Only process if we have significant signal amplitude
    if(amplitude > 500) {  // You might need to adjust this threshold
        uint16_t threshold = (max_val + min_val) / 2;
        // Increased hysteresis for better noise immunity
        uint16_t hysteresis = amplitude / 10;  // 10% hysteresis (increased from 5%)
        uint16_t upper_threshold = threshold + hysteresis;
        uint16_t lower_threshold = threshold - hysteresis;
        // Variables for edge detection
        uint32_t first_crossing = 0;
        uint32_t last_crossing = 0;
        int crossing_count = 0;
        bool above = capture_buf[0] > threshold;

        // Find first and last valid crossings
        for(int i = 1; i < CAPTURE_DEPTH; i++) {
            if(capture_buf[i] != 0) {  // Skip invalid samples
                if(above && capture_buf[i] < lower_threshold) {
                    if(first_crossing == 0) first_crossing = i;
                    last_crossing = i;
                    crossing_count++;
                    above = false;
                }
                else if(!above && capture_buf[i] > upper_threshold) {
                    if(first_crossing == 0) first_crossing = i;
                    last_crossing = i;
                    crossing_count++;
                    above = true;
                }
            }
        }
        // Calculate frequency only if we have enough valid crossings
        if(crossing_count >= 4) {  // Increased minimum crossings for better accuracy
            // Calculate actual time period between first and last crossing
            float sample_period = 1.0f / 10000.0f;  // Correct time per sample (1/10kHz)
            float measurement_time = (last_crossing - first_crossing) * sample_period;
            float cycles = (float)(crossing_count) / 2.0f;  // Convert crossings to complete cycles
            frequency = cycles / measurement_time;
            // Sanity check on frequency range
            if(frequency > 1.0f && frequency < 5000.0f) {
                printf("Analog Analysis:\n");
                printf("  Frequency: %.1f Hz\n", frequency);
            }
        }
    }
    return frequency;
}
void dma_handler() {
    // Clear the interrupt request
    dma_hw->ints0 = 1u << dma_chan;
    // Stop ADC before analyzing
    adc_run(false);
    // Analyze the captured data
    float freq = analyze_capture();

    // Prepare for next capture
    adc_fifo_drain();
    // Configure next DMA transfer
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);
    
    // Start next capture
    dma_channel_configure(
        dma_chan,
        &cfg,
        capture_buf,    // dst
        &adc_hw->fifo,  // src
        CAPTURE_DEPTH,  // transfer count
        true            // start immediately
    );
    // Restart ADC
    adc_run(true);
}
// Modify setup_adc_capture() to ensure proper initialization
void setup_adc_capture() {
    printf("Initializing ADC and DMA...\n");
    adc_gpio_init(26);
    adc_init();
    adc_select_input(0);
    // Modified ADC settings
    adc_fifo_setup(
        true,    // Write each conversion to FIFO
        true,    // Enable DMA requests
        1,       // DREQ when at least 1 sample present
        false,   // Disable error bit
        false    // Don't shift samples - keep full 12 bits
    );
    // Slower sampling rate for better low frequency detection
    // 48MHz / 4800 = 10kHz sampling rate
    adc_set_clkdiv(4800);  
    // Clear any pending samples
    adc_fifo_drain();
    // Rest of setup remains the same...
    dma_chan = dma_claim_unused_channel(true);
    printf("Using DMA channel %d\n", dma_chan);
    dma_channel_set_irq0_enabled(dma_chan, true);
    irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
    irq_set_enabled(DMA_IRQ_0, true);
    dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
    channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
    channel_config_set_read_increment(&cfg, false);
    channel_config_set_write_increment(&cfg, true);
    channel_config_set_dreq(&cfg, DREQ_ADC);
    printf("Starting first DMA transfer...\n");
    dma_channel_configure(
        dma_chan,
        &cfg,
        capture_buf,    
        &adc_hw->fifo,  
        CAPTURE_DEPTH,  
        true           
    );
    adc_run(true);
    printf("ADC and DMA initialization complete\n");
}
int main() {
    stdio_init_all();
    sleep_ms(2000);
    // Initialize GPIOs for digital and PWM inputs
    gpio_init(DIGITAL_PIN);
    gpio_init(PWM_PIN);
    gpio_set_dir(DIGITAL_PIN, GPIO_IN);
    gpio_set_dir(PWM_PIN, GPIO_IN);
    // Setup GPIO interrupts
    gpio_set_irq_enabled_with_callback(DIGITAL_PIN, 
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled_with_callback(PWM_PIN, 
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    // Remove duplicate ADC setup and use setup_adc_capture() instead
    setup_adc_capture();
    printf("Signal Analyzer Ready\n");
    printf("- Digital (GP2): HIGH/LOW states\n");
    printf("- PWM (GP7): Frequency and Duty Cycle\n");
    printf("- Analog (GP26): Frequency measurement enabled\n");
    while(1) {
        sleep_ms(100);
    }
    return 0;
}