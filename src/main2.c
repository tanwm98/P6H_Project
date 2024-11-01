#include "pico/stdlib.h"
#include "buddy2/signal_analyzer.h"

int main() {
    // int main() {

//     stdio_init_all();
//     sleep_ms(10000);  // Delay to allow the serial console to connect 

//     FRESULT fr = initialiseSD();
//     if (fr != FR_OK) {
//         printf("Failed to initialize SD card\n");
//         return -1;  // Stop if SD card initialization fails
//     }

//     FIL file;  // File object
//     const char* filename = "temperature_log.txt";  // Filename for temperature logs
//     FRESULT fr1;

//     // Open file for appending temperature data
//     fr1 = f_open(&file, filename, FA_OPEN_APPEND | FA_WRITE);
//     if (fr1 != FR_OK) {
//         printf("f_open error: %d\n", (int)fr);
//         return 0;
//     }

//     return 0;
// }
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