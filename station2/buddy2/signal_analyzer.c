// #include "signal_analyzer.h"


// // Global variables
// static uint16_t capture_buf[CAPTURE_DEPTH];
// static uint dma_chan;
// static volatile DigitalSignal digital_signal = {0};
// static volatile PWMMetrics pwm_metrics = {0};
// static volatile float analog_frequency = 0.0f;

// // Forward declarations of internal functions
// static void gpio_callback(uint gpio, uint32_t events);
// static float analyze_capture(void);
// static void dma_handler(void);
// static void setup_adc_capture(void);

// // GPIO interrupt handler
// static void gpio_callback(uint gpio, uint32_t events) {
//     uint32_t now = time_us_32();
//     if(gpio == DIGITAL_PIN) {
//         if(events & GPIO_IRQ_EDGE_RISE) {
//             digital_signal.last_rise_time = now;
//             digital_signal.current_state = true;
//             printf("Digital Signal: HIGH at %lu us\n", now);
        
//             if(digital_signal.last_fall_time != 0) {
//                 uint32_t low_width = now - digital_signal.last_fall_time;
//                 printf("LOW pulse width: %lu us\n", low_width);
//             }
//         }
//         else if(events & GPIO_IRQ_EDGE_FALL) {
//             digital_signal.last_fall_time = now;
//             digital_signal.current_state = false;

//             if(digital_signal.last_rise_time != 0) {
//                 digital_signal.pulse_width = now - digital_signal.last_rise_time;
//                 printf("Digital Signal: LOW at %lu us\n", now);
//                 printf("HIGH pulse width: %lu us\n", digital_signal.pulse_width);
//             }
//         }
//     }
//     else if(gpio == PWM_PIN) {
//         if(events & GPIO_IRQ_EDGE_RISE) {
//             if(pwm_metrics.last_rise != 0) {
//                 pwm_metrics.period = now - pwm_metrics.last_rise;
//                 pwm_metrics.frequency = 1000000.0f / pwm_metrics.period;
//             }
//             pwm_metrics.last_rise = now;
//         } else if(events & GPIO_IRQ_EDGE_FALL) {
//             if(pwm_metrics.last_rise != 0) {
//                 uint32_t high_time = now - pwm_metrics.last_rise;
//                 if(pwm_metrics.period > 0) {
//                     pwm_metrics.duty_cycle = (float)high_time * 100.0f / pwm_metrics.period;
//                 }
//             }
//             pwm_metrics.last_fall = now;
//             printf("PWM - Frequency: %.2f Hz, Duty Cycle: %.1f%%\n", 
//                    pwm_metrics.frequency, pwm_metrics.duty_cycle);
//         }
//     }
// }

// static float analyze_capture(void) {
//     uint16_t max_val = 0;
//     uint16_t min_val = 4096;
//     for(int i = 0; i < CAPTURE_DEPTH; i++) {
//         if(capture_buf[i] > max_val) max_val = capture_buf[i];
//         if(capture_buf[i] < min_val && capture_buf[i] != 0) min_val = capture_buf[i];
//     }
//     uint16_t amplitude = max_val - min_val;
//     float frequency = 0.0f;

//     if(amplitude > 500) {
//         uint16_t threshold = (max_val + min_val) / 2;
//         uint16_t hysteresis = amplitude / 10;
//         uint16_t upper_threshold = threshold + hysteresis;
//         uint16_t lower_threshold = threshold - hysteresis;
//         uint32_t first_crossing = 0;
//         uint32_t last_crossing = 0;
//         int crossing_count = 0;
//         bool above = capture_buf[0] > threshold;

//         for(int i = 1; i < CAPTURE_DEPTH; i++) {
//             if(capture_buf[i] != 0) {
//                 if(above && capture_buf[i] < lower_threshold) {
//                     if(first_crossing == 0) first_crossing = i;
//                     last_crossing = i;
//                     crossing_count++;
//                     above = false;
//                 }
//                 else if(!above && capture_buf[i] > upper_threshold) {
//                     if(first_crossing == 0) first_crossing = i;
//                     last_crossing = i;
//                     crossing_count++;
//                     above = true;
//                 }
//             }
//         }

//         if(crossing_count >= 4) {
//             float sample_period = 1.0f / 10000.0f;
//             float measurement_time = (last_crossing - first_crossing) * sample_period;
//             float cycles = (float)(crossing_count) / 2.0f;
//             frequency = cycles / measurement_time;
//             if(frequency > 1.0f && frequency < 5000.0f) {
//                 printf("Analog Analysis:\n");
//                 printf("  Frequency: %.1f Hz\n", frequency);
//                 analog_frequency = frequency;
//             }
//         }
//     }
//     return frequency;
// }
// static void setup_adc_capture(void) {
//     printf("Initializing ADC and DMA...\n");
    
//     // Initialize ADC
//     adc_gpio_init(ANALOG_PIN);
//     adc_init();
//     adc_select_input(0);
    
//     adc_fifo_setup(
//         true,    // Write each conversion to FIFO
//         true,    // Enable DMA requests
//         1,       // DREQ when at least 1 sample present
//         false,   // Disable error bit
//         false    // Don't shift samples
//     );
    
//     adc_set_clkdiv(4800);
//     adc_fifo_drain();
    
//     // Claim an unused DMA channel dynamically
//     dma_chan = dma_claim_unused_channel(true);
//     if (dma_chan < 0) {
//         printf("Error: Could not claim a DMA channel\n");
//         return;
//     }
//     printf("Using DMA channel %d\n", dma_chan);
    
//     // Configure DMA
//     dma_channel_set_irq0_enabled(dma_chan, true);
//     irq_set_exclusive_handler(DMA_IRQ_0, dma_handler);
//     irq_set_enabled(DMA_IRQ_0, true);
    
//     dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
//     channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
//     channel_config_set_read_increment(&cfg, false);
//     channel_config_set_write_increment(&cfg, true);
//     channel_config_set_dreq(&cfg, DREQ_ADC);
    
//     printf("Starting first DMA transfer...\n");
//     dma_channel_configure(
//         dma_chan,
//         &cfg,
//         capture_buf,
//         &adc_hw->fifo,
//         CAPTURE_DEPTH,
//         true
//     );
//     adc_run(true);
//     printf("ADC and DMA initialization complete\n");
// }

// // Modify dma_handler to use the claimed channel
// static void dma_handler(void) {
//     if (dma_chan < 0) return;  // Safety check
    
//     dma_hw->ints0 = 1u << dma_chan;
//     adc_run(false);
//     analyze_capture();
//     adc_fifo_drain();

//     dma_channel_config cfg = dma_channel_get_default_config(dma_chan);
//     channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
//     channel_config_set_read_increment(&cfg, false);
//     channel_config_set_write_increment(&cfg, true);
//     channel_config_set_dreq(&cfg, DREQ_ADC);
    
//     dma_channel_configure(
//         dma_chan,
//         &cfg,
//         capture_buf,
//         &adc_hw->fifo,
//         CAPTURE_DEPTH,
//         true
//     );
//     adc_run(true);
// }

// // Public functions
// void signal_analyzer_init(void) {
//     // Initialize GPIOs for digital and PWM inputs
//     gpio_init(DIGITAL_PIN);
//     gpio_init(PWM_PIN);
//     gpio_set_dir(DIGITAL_PIN, GPIO_IN);
//     gpio_set_dir(PWM_PIN, GPIO_IN);
    
//     // Setup GPIO interrupts
//     gpio_set_irq_enabled_with_callback(DIGITAL_PIN, 
//         GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
//     gpio_set_irq_enabled_with_callback(PWM_PIN, 
//         GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    
//     // Initialize ADC and DMA
//     setup_adc_capture();
    
//     printf("Signal Analyzer Ready\n");
//     printf("- Digital (GP2): HIGH/LOW states\n");
//     printf("- PWM (GP7): Frequency and Duty Cycle\n");
//     printf("- Analog (GP26): Frequency measurement enabled\n");
// }

// float get_analog_frequency(void) {
//     return analog_frequency;
// }

// DigitalSignal get_digital_signal_state(void) {
//     DigitalSignal signal;
//     signal = digital_signal;
//     return signal;
// }

// PWMMetrics get_pwm_metrics(void) {
//     PWMMetrics metrics;
//     metrics = pwm_metrics;
//     return metrics;
// }