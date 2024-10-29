#include "pico/stdlib.h"
#include "hardware/adc.h"
#include "hardware/timer.h"
#include "hardware/pwm.h"
#include "hardware/irq.h"
#include <stdio.h>

#define PWM_PIN 7           // GPIO pin for PWM signal
#define ADC_PIN 26          // GPIO pin for ADC input
#define BUTTON_PIN 20       // GPIO pin for button input
#define SAMPLE_RATE_MS 10   // Interval in ms for ADC sampling (100 Hz rate)
#define NUM_SAMPLES 100     // Number of samples to capture for waveform

#define SINGLE_PULSE_DURATION_MS 1 // Duration of a single PWM pulse in milliseconds
#define ADC_MAX 4095.0f            // Max ADC value for conversion to voltage
#define REF_VOLTAGE 3.3f           // Reference voltage for ADC conversion

bool pulse_generated = false;      // Flag to indicate if pulse was generated
uint16_t waveform_data[NUM_SAMPLES];
int sample_index = 0;
struct repeating_timer adc_timer;

// Inline function for PWM calculations
inline float calculate_duty_cycle(uint16_t pwm_level, uint16_t pwm_wrap) {
    return (pwm_level / (float)pwm_wrap) * 100;
}

inline float calculate_time_us(uint16_t pwm_value, uint32_t clock_freq, float wrap_plus_one) {
    return (pwm_value / (float)clock_freq) * wrap_plus_one * 1e6;
}

// Function to capture the PWM signal data
void capture_pwm_signal(uint slice_num) {
    uint16_t pwm_wrap = pwm_hw->slice[slice_num].top;
    uint16_t pwm_level = pwm_hw->slice[slice_num].cc;
    const uint32_t clock_freq = 125000000;
    const float wrap_plus_one = pwm_wrap + 1;

    uint32_t frequency = clock_freq / wrap_plus_one;
    float duty_cycle = calculate_duty_cycle(pwm_level, pwm_wrap);
    float high_time = calculate_time_us(pwm_level, clock_freq, wrap_plus_one);
    float low_time = calculate_time_us(pwm_wrap - pwm_level, clock_freq, wrap_plus_one);

    printf("PWM - Sent single pulse via GP20\n");
    printf("PWM - Frequency: %u Hz, Duty Cycle: %.2f%%, High Time: %.2f us, Low Time: %.2f us\n", 
           frequency, duty_cycle, high_time, low_time);
}

// Function to capture ADC reading and print voltage
void capture_adc_reading() {
    uint16_t adc_value = adc_read();
    float voltage = (adc_value / ADC_MAX) * REF_VOLTAGE;
    printf("ADC - Value: %u, Voltage: %.2f V\n", adc_value, voltage);
}

// Interrupt-based function to sample ADC at a regular interval
bool adc_timer_callback(struct repeating_timer *t) {
    if (sample_index < NUM_SAMPLES) {
        waveform_data[sample_index++] = adc_read();
    }
    return sample_index < NUM_SAMPLES;
}

// Function to generate a single pulse and capture both PWM and ADC data
void generate_and_capture(uint slice_num) {
    pwm_set_enabled(slice_num, true);  // Enable PWM on GP7
    sleep_ms(SINGLE_PULSE_DURATION_MS); // Wait for the duration of one pulse
    capture_pwm_signal(slice_num);      // Capture the PWM data immediately after generation
    pwm_set_enabled(slice_num, false);  // Disable PWM to end the pulse

    capture_adc_reading();              // Capture ADC data
    pulse_generated = true;             // Set flag indicating pulse and ADC read are complete
}

int main(void) 
{
    stdio_init_all();

    gpio_set_function(PWM_PIN, GPIO_FUNC_PWM);
    uint slice_num = pwm_gpio_to_slice_num(PWM_PIN);
    pwm_set_wrap(slice_num, 9999);  
    pwm_set_chan_level(slice_num, PWM_CHAN_A, 5000); 
    pwm_set_enabled(slice_num, false);

    adc_init();
    adc_gpio_init(ADC_PIN);
    adc_select_input(0);

    gpio_init(BUTTON_PIN);
    gpio_set_dir(BUTTON_PIN, GPIO_IN);
    gpio_pull_up(BUTTON_PIN);

    while (true) 
    {
        if (gpio_get(BUTTON_PIN) == 0 && !pulse_generated) { 
            generate_and_capture(slice_num); 
            sleep_ms(10); // Debounce delay
        }

        if (gpio_get(BUTTON_PIN) == 1) { 
            pulse_generated = false;
        }

        tight_loop_contents(); // Keep the program running
    }
}
