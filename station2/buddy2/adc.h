#ifndef ADC_H
#define ADC_H

#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/dma.h"
#include <stdbool.h>

#define DEFAULT_CAPTURE_DEPTH 10000
#define DEFAULT_ANALOG_PIN 26


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

// Public function declarations
void adc_analyzer_init(void);
void adc_cleanup(void);
bool is_adc_capturing(void);
void adc_start_capture(void);
void adc_stop_capture(void);
float get_last_frequency(void);
bool is_transfer_complete(void);
void clear_transfer_complete(void);
float analyze_current_capture(void);

#endif // ADC_H