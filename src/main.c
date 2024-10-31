#include "pico/stdlib.h"
#include "buddy2/signal_analyzer.h"

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Give time for USB serial to initialize
    signal_analyzer_init();
    while(1) {
    sleep_ms(100);
    // You can add your other functionalities here
    // Access signal measurements using:
    // get_analog_frequency()
    // get_digital_signal_state()
    // get_pwm_metrics()    
    }
    return 0;
}