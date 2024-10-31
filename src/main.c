#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include <stdio.h>

#define DIGITAL_PIN 2      // GP2 for digital pulses
#define PWM_PIN 7         // GP7 for PWM analysis
#define ANALOG_PIN 26     // GP26 (ADC0) for analog signals
#define DETECTION_THRESHOLD 100  // ADC threshold for connection detection
#define SAMPLE_PERIOD_MS 100000    // Check for connections every 100ms
#define ADC_SAMPLE_RATE 50000    // 50kHz sampling for analog
#define ADC_SAMPLES 5000        // Number of samples for frequency detection

typedef struct {
    bool digital_connected;
    bool pwm_connected;
    bool analog_connected;
    uint slice_num;  // For PWM
} ConnectionStatus;

typedef struct {
    float frequency;
    float duty_cycle;
} SignalMetrics;


// Function to check if analog pin is connected (has signal)
bool check_analog_connection() {
    uint16_t max_value = 0;
    uint16_t min_value = 4095;
    
    // Take several samples to detect signal presence
    for(int i = 0; i < 100; i++) {
        uint16_t value = adc_read();
        if(value > max_value) max_value = value;
        if(value < min_value) min_value = value;
        sleep_us(100);
    }
    
    // If there's significant variation or DC level, consider connected
    return (max_value - min_value > DETECTION_THRESHOLD) || (min_value > DETECTION_THRESHOLD);
}

// Function to check if PWM pin is connected (has transitions)
bool check_pwm_connection(uint slice_num) {
    uint32_t last_value = 0;
    uint32_t transitions = 0;
    
    // Monitor for transitions
    for(int i = 0; i < 1000; i++) {
        uint32_t current_value = gpio_get(PWM_PIN);
        if(current_value != last_value) {
            transitions++;
        }
        last_value = current_value;
        sleep_us(10);
    }
    
    return transitions > 0;
}

// Function to check if digital pin is connected (has transitions)
bool check_digital_connection() {
    uint32_t last_value = 0;
    uint32_t transitions = 0;
    
    // Monitor for transitions
    for(int i = 0; i < 1000; i++) {
        uint32_t current_value = gpio_get(DIGITAL_PIN);
        if(current_value != last_value) {
            transitions++;
        }
        last_value = current_value;
        sleep_us(10);
    }
    
    return transitions > 0;
}

void analyze_digital() {
    uint32_t last_value = gpio_get(DIGITAL_PIN);
    uint32_t last_rise_time = 0;
    uint32_t last_fall_time = 0;
    uint32_t current_time;
    bool waiting_for_next_rise = false;
    
     while(1) {
        uint32_t current_value = gpio_get(DIGITAL_PIN);
        current_time = to_us_since_boot(get_absolute_time());
        
        if(current_value != last_value) {
            if(current_value) {  // RISE
                printf("RISE (LOW->HIGH) at %lu us\n", current_time);
                if(last_rise_time != 0) {
                    uint32_t period = current_time - last_rise_time;
                    printf("Period: %lu us (%.2f Hz)\n", period, 1000000.0f/period);
                }
                last_rise_time = current_time;
                waiting_for_next_rise = false;
            } else {  // FALL
                printf("FALL (HIGH->LOW) at %lu us\n", current_time);
                if(!waiting_for_next_rise && last_rise_time != 0) {
                    uint32_t pulse_width = current_time - last_rise_time;
                    printf("Pulse width: %lu us\n", pulse_width);
                }
                last_fall_time = current_time;
                waiting_for_next_rise = true;
            }
        }
        
        last_value = current_value;
        sleep_us(100);  // Small delay to prevent tight polling
    }
}

// Function to analyze PWM signal
void analyze_pwm(uint slice_num) {
    uint32_t total_samples = 0;
    uint32_t high_samples = 0;
    uint32_t edge_count = 0;
    uint32_t last_edge = 0;
    uint32_t total_period = 0;
    uint32_t last_value = gpio_get(PWM_PIN);
    
    uint32_t start_time = to_us_since_boot(get_absolute_time());
    
    // Sample for 100ms
    while (to_us_since_boot(get_absolute_time()) - start_time < 100000) {
        uint32_t current_value = gpio_get(PWM_PIN);
        uint32_t current_time = to_us_since_boot(get_absolute_time());
        
        // Count edges for frequency
        if (current_value != last_value) {
            if (current_value) {  // Rising edge
                if (last_edge != 0) {
                    total_period += current_time - last_edge;
                    edge_count++;
                }
                last_edge = current_time;
            }
        }
        
        // Count high samples for duty cycle
        if (current_value) {
            high_samples++;
        }
        total_samples++;
        
        last_value = current_value;
        sleep_us(1);
    }
    
    float frequency = edge_count > 0 ? 1000000.0f / (total_period / edge_count) : 0;
    float duty_cycle = (float)high_samples * 100.0f / total_samples;
    
    printf("PWM Analysis (GP7):\n");
    printf("- Frequency: %.1f Hz\n", frequency);
    printf("- Duty Cycle: %.1f%%\n", duty_cycle);
}


// Improved analog signal analysis with frequency detection
void analyze_analog() {
    uint16_t samples[ADC_SAMPLES];
    uint32_t crossings = 0;
    float threshold;
    uint32_t start_time = to_us_since_boot(get_absolute_time());
    
    // Gather samples
    for(int i = 0; i < ADC_SAMPLES; i++) {
        samples[i] = adc_read();
        sleep_us(20);  // Maintain ~50kHz sample rate
    }
    
    // Calculate average for zero-crossing detection
    uint32_t sum = 0;
    uint16_t max_value = 0;
    uint16_t min_value = 4095;
    
    for(int i = 0; i < ADC_SAMPLES; i++) {
        sum += samples[i];
        if(samples[i] > max_value) max_value = samples[i];
        if(samples[i] < min_value) min_value = samples[i];
    }
    
    threshold = sum / ADC_SAMPLES;
    
    // Count zero crossings for frequency calculation
    bool above = samples[0] > threshold;
    for(int i = 1; i < ADC_SAMPLES; i++) {
        bool current_above = samples[i] > threshold;
        if(above != current_above) {
            crossings++;
            above = current_above;
        }
    }
    
    uint32_t measurement_time = to_us_since_boot(get_absolute_time()) - start_time;
    float frequency = (float)crossings * 500000.0f / measurement_time; // Convert to Hz (divide by 2 for full cycles)
    float vpp = (max_value - min_value) * 3.3f / 4095.0f;
    
    printf("Analog Analysis (GP26):\n");
    printf("- Frequency: %.1f Hz\n", frequency);
    printf("- Peak-to-peak voltage: %.3f V\n", vpp);
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    printf("\nDynamic Signal Monitor\n");
    
    // Initialize pins
    gpio_init(DIGITAL_PIN);
    gpio_init(PWM_PIN);
    gpio_set_dir(DIGITAL_PIN, GPIO_IN);
    gpio_set_dir(PWM_PIN, GPIO_IN);
    
    // Initialize ADC
    adc_init();
    adc_gpio_init(ANALOG_PIN);
    adc_select_input(0);  // GP26 is ADC0
    
    ConnectionStatus status = {false, false, false, 0};
    absolute_time_t last_check = get_absolute_time();
    
    while(true) {
        // Check for new connections every 100ms
        if(absolute_time_diff_us(last_check, get_absolute_time()) >= SAMPLE_PERIOD_MS) {
            bool new_digital = check_digital_connection();
            if(new_digital != status.digital_connected) {
                status.digital_connected = new_digital;
                if(new_digital) {
                    printf("\nDigital signal detected on GP2!\n");
                }
            }
            
            bool new_pwm = check_pwm_connection(status.slice_num);
            if(new_pwm != status.pwm_connected) {
                status.pwm_connected = new_pwm;
                if(new_pwm) {
                    printf("\nPWM signal detected on GP7!\n");
                    analyze_pwm(status.slice_num);
                }
            }
            
            bool new_analog = check_analog_connection();
            if(new_analog != status.analog_connected) {
                status.analog_connected = new_analog;
                if(new_analog) {
                    printf("\nAnalog signal detected on GP26!\n");
                    analyze_analog();
                }
            }
            
            last_check = get_absolute_time();
        }
        
        if(status.digital_connected) {
            analyze_digital();  // Original GP2 analysis remains unchanged
        }
        if(status.pwm_connected) {
            analyze_pwm(status.slice_num);
        }
        if(status.analog_connected) {
            analyze_analog();
        }
        
        sleep_ms(10);
    }
    
    return 0;
}