// // signal_analyzer.h
// #ifndef SIGNAL_ANALYZER_H
// #define SIGNAL_ANALYZER_H

// #include "pico/stdlib.h"
// #include "hardware/gpio.h"
// #include "hardware/adc.h"
// #include "hardware/pwm.h"
// #include <stdio.h>
// #include <string.h>

// // Pin definitions
// #define DIGITAL_PIN 2
// #define PWM_PIN 7
// #define ANALOG_PIN 26

// // Function prototypes
// // void signal_analyzer_init(void);
// void signal_analyzer_deinit(void);
// bool start_pwm_analysis(void);
// bool stop_pwm_analysis(void);
// bool start_analog_analysis(void);
// bool stop_analog_analysis(void);
// float get_pwm_frequency(void);
// float get_pwm_duty_cycle(void);
// float get_analog_frequency(void);
// float get_analog_duty_cycle(void);

// // Result structure
// typedef struct {
//     float frequency;
//     float duty_cycle;
//     bool valid;
// } SignalMeasurement;

// #endif // SIGNAL_ANALYZER_H