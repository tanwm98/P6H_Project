// // signal_analyzer.c
// #include "signal_analyzer.h"
// #include "hardware/gpio.h"
// #include "hardware/adc.h"
// #include "hardware/dma.h"
// #include "hardware/irq.h"
// #include <stdio.h>
// #include <stdlib.h>

// #define ADC_SAMPLE_RATE 50000
// #define ADC_SAMPLES 5000

// static void gpio_irq_handler(uint gpio, uint32_t events, void* user_data) {
//     SignalAnalyzer* analyzer = (SignalAnalyzer*)user_data;
//     uint32_t current_time = to_us_since_boot(get_absolute_time());
    
//     if (events & GPIO_IRQ_EDGE_RISE) {
//         if (analyzer->metrics.last_rise_time != 0) {
//             uint32_t period = current_time - analyzer->metrics.last_rise_time;
//             analyzer->metrics.frequency = 1000000.0f / period;
//         }
//         analyzer->metrics.last_rise_time = current_time;
//         analyzer->edge_timestamps[analyzer->edge_count++ & 0xFF] = current_time;
//     }
    
//     if (events & GPIO_IRQ_EDGE_FALL) {
//         analyzer->metrics.last_fall_time = current_time;
//         if (analyzer->metrics.last_rise_time != 0) {
//             uint32_t high_time = current_time - analyzer->metrics.last_rise_time;
//             uint32_t period = current_time - analyzer->edge_timestamps[(analyzer->edge_count - 2) & 0xFF];
//             analyzer->metrics.duty_cycle = (float)high_time * 100.0f / period;
//         }
//     }
    
//     analyzer->metrics.signal_present = true;
    
//     // Notify callback if registered
//     if (analyzer->on_update) {
//         analyzer->on_update(&analyzer->metrics);
//     }
// }

// static void adc_dma_handler(void* user_data) {
//     SignalAnalyzer* analyzer = (SignalAnalyzer*)user_data;
    
//     if (dma_channel_get_irq0_status(analyzer->dma_channel)) {
//         dma_channel_acknowledge_irq0(analyzer->dma_channel);
//         analyzer->adc_complete = true;
        
//         // Process ADC data
//         uint32_t sum = 0;
//         uint16_t max_val = 0;
//         uint16_t min_val = 4095;
//         uint32_t crossings = 0;
        
//         for (int i = 0; i < ADC_SAMPLES; i++) {
//             sum += analyzer->adc_buf[i];
//             if (analyzer->adc_buf[i] > max_val) max_val = analyzer->adc_buf[i];
//             if (analyzer->adc_buf[i] < min_val) min_val = analyzer->adc_buf[i];
//         }
        
//         float avg = (float)sum / ADC_SAMPLES;
//         bool above = analyzer->adc_buf[0] > avg;
        
//         for (int i = 1; i < ADC_SAMPLES; i++) {
//             bool current_above = analyzer->adc_buf[i] > avg;
//             if (above != current_above) {
//                 crossings++;
//                 above = current_above;
//             }
//         }
        
//         analyzer->metrics.frequency = (float)crossings * ADC_SAMPLE_RATE / (2 * ADC_SAMPLES);
//         analyzer->metrics.signal_present = true;
        
//         // Notify callback if registered
//         if (analyzer->on_update) {
//             analyzer->on_update(&analyzer->metrics);
//         }
        
//         // Restart DMA transfer
//         dma_channel_set_write_addr(analyzer->dma_channel, analyzer->adc_buf, true);
//     }
// }

// SignalAnalyzer* signal_analyzer_init_digital(uint gpio_pin, void (*callback)(const SignalMetrics*)) {
//     SignalAnalyzer* analyzer = calloc(1, sizeof(SignalAnalyzer));
//     if (!analyzer) return NULL;
    
//     analyzer->gpio_pin = gpio_pin;
//     analyzer->on_update = callback;
    
//     gpio_init(gpio_pin);
//     gpio_set_dir(gpio_pin, GPIO_IN);
    
//     return analyzer;
// }

// SignalAnalyzer* signal_analyzer_init_analog(uint gpio_pin, uint adc_channel, void (*callback)(const SignalMetrics*)) {
//     SignalAnalyzer* analyzer = calloc(1, sizeof(SignalAnalyzer));
//     if (!analyzer) return NULL;
    
//     analyzer->gpio_pin = gpio_pin;
//     analyzer->adc_channel = adc_channel;
//     analyzer->on_update = callback;
    
//     // Allocate ADC buffer
//     analyzer->adc_buf = calloc(ADC_SAMPLES, sizeof(uint16_t));
//     if (!analyzer->adc_buf) {
//         free(analyzer);
//         return NULL;
//     }
    
//     // Initialize ADC
//     adc_init();
//     adc_gpio_init(gpio_pin);
//     adc_select_input(adc_channel);
    
//     // Get a free DMA channel
//     analyzer->dma_channel = dma_claim_unused_channel(true);
    
//     return analyzer;
// }

// void signal_analyzer_start(SignalAnalyzer* analyzer) {
//     if (!analyzer) return;
    
//     // For digital signals
//     if (!analyzer->adc_buf) {
//         gpio_set_irq_enabled_with_callback(analyzer->gpio_pin,
//             GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL,
//             true,
//             (gpio_irq_callback_t)gpio_irq_handler);
//     }
//     // For analog signals
//     else {
//         // Configure DMA
//         dma_channel_config cfg = dma_channel_get_default_config(analyzer->dma_channel);
//         channel_config_set_transfer_data_size(&cfg, DMA_SIZE_16);
//         channel_config_set_read_increment(&cfg, false);
//         channel_config_set_write_increment(&cfg, true);
//         channel_config_set_dreq(&cfg, DREQ_ADC);
        
//         dma_channel_configure(
//             analyzer->dma_channel,
//             &cfg,
//             analyzer->adc_buf,
//             &adc_hw->fifo,
//             ADC_SAMPLES,
//             true
//         );
        
//         // Enable DMA interrupt
//         dma_channel_set_irq0_enabled(analyzer->dma_channel, true);
//         irq_set_exclusive_handler(DMA_IRQ_0, (irq_handler_t)adc_dma_handler);
//         irq_set_enabled(DMA_IRQ_0, true);
        
//         // Start ADC
//         adc_run(true);
//     }
// }

// void signal_analyzer_stop(SignalAnalyzer* analyzer) {
//     if (!analyzer) return;
    
//     if (!analyzer->adc_buf) {
//         gpio_set_irq_enabled(analyzer->gpio_pin, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
//     } else {
//         adc_run(false);
//         dma_channel_abort(analyzer->dma_channel);
//     }
// }

// void signal_analyzer_get_metrics(SignalAnalyzer* analyzer, SignalMetrics* metrics) {
//     if (!analyzer || !metrics) return;
    
//     // Critical section to ensure atomic copy
//     uint32_t save = save_and_disable_interrupts();
//     *metrics = analyzer->metrics;
//     restore_interrupts(save);
// }

// void signal_analyzer_deinit(SignalAnalyzer* analyzer) {
//     if (!analyzer) return;
    
//     signal_analyzer_stop(analyzer);
    
//     if (analyzer->adc_buf) {
//         free(analyzer->adc_buf);
//         dma_channel_unclaim(analyzer->dma_channel);
//     }
    
//     free(analyzer);
// }