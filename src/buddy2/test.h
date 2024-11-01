// #ifndef SIGNAL_ANALYZER_H
// #define SIGNAL_ANALYZER_H

// #include "pico/stdlib.h"
// #include "hardware/gpio.h"
// #include "hardware/adc.h"
// #include "hardware/pwm.h"
// #include "hardware/dma.h"
// #include "hardware/timer.h"
// #include "pico/lock_core.h"
// #include <stdio.h>
// #include <string.h>


// #define DIGITAL_PIN 2      // GP2 for digital pulses
// #define PWM_PIN 7         // GP7 for PWM analysis
// #define ANALOG_PIN 26     // GP26 (ADC0) for analog signals
// #define ADC_CLOCK_DIV 0   // Run ADC at full speed
// #define ADC_RANGE 4096    // 12-bit ADC
// #define MIN_AMPLITUDE 100  // Minimum peak-to-peak amplitude
// #define CAPTURE_DEPTH 5000 // Capture window size
// #define MIN_FREQ 0.5f     // Minimum frequency to detect
// #define MAX_FREQ 300.0f   // Maximum frequency to detect
// #define SAMPLE_RATE 1000.0f // 2kHz sampling rate


// #define MAX_PULSES 10
// static void gpio_callback(uint gpio, uint32_t events);
// bool is_capture_complete(void);
// void reset_capture(void);
// bool save_captured_pulses(void);


// #define BUTTON_PIN 20     // GP20 for global control
// #define DEBOUNCE_TIME 200000  // 200ms debounce in microseconds

// // Add these to the existing declarations
// extern volatile bool is_system_active;
// extern volatile uint32_t last_button_press;

// // Digital signal measurements
// typedef struct {
//     uint32_t last_rise_time;
//     uint32_t last_fall_time;
//     uint32_t pulse_width;
//     bool current_state;
// } DigitalSignal;

// // PWM measurements
// typedef struct {
//     float frequency;
//     float duty_cycle;
//     uint32_t last_rise;
//     uint32_t last_fall;
//     uint32_t period;
// } PWMMetrics;

// // Function declarations
// void signal_analyzer_init(void);
// float get_analog_frequency(void);
// DigitalSignal get_digital_signal_state(void);
// PWMMetrics get_pwm_metrics(void);


// // Add these function declarations
// bool is_capture_complete(void);
// void reset_capture(void);
// bool save_captured_pulses(void);

// #endif // SIGNAL_ANALYZER_H