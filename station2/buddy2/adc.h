#ifndef ADC_H
#define ADC_H

#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <stdbool.h>

// Configuration constants
#define DEFAULT_CAPTURE_DEPTH 10000
#define DEFAULT_BUTTON_PIN 20
#define DEFAULT_ANALOG_PIN 26

typedef struct {
    uint16_t* capture_buf;
    uint capture_depth;
    uint dma_chan;
    uint button_pin;
    uint analog_pin;
    volatile bool capturing;
    volatile bool transfer_complete;
    volatile float last_frequency;
    volatile bool continuous_mode;  // Added for continuous capture
} ADC_Config;

// Initialize ADC configuration with default values
void adc_init_config(ADC_Config* config);

// Initialize ADC hardware with given configuration
bool adc_init_hardware(ADC_Config* config);

// Start capture
void adc_start_capture(ADC_Config* config);

// Stop capture
void adc_stop_capture(ADC_Config* config);

// Analyze captured data
float adc_analyze_capture(ADC_Config* config);

// Clean up ADC resources
void adc_cleanup(ADC_Config* config);

// Button callback handler
void adc_button_handler(uint gpio, uint32_t events);

#endif // ADC_H