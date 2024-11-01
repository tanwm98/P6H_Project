#include "buddy2/adc.h"
#include <stdio.h>

int main() {
    stdio_init_all();
    sleep_ms(2000);  // Give time for serial to initialize
    
    // Create and initialize ADC configuration
    ADC_Config adc_config;
    adc_init_config(&adc_config);
    
    // Initialize ADC hardware
    if (!adc_init_hardware(&adc_config)) {
        printf("Failed to initialize ADC hardware\n");
        return -1;
    }
    
    printf("\nSystem Ready - Press button to start/stop continuous capture\n");
    
    while (true) {
        if (adc_config.transfer_complete) {
            adc_analyze_capture(&adc_config);
            adc_config.transfer_complete = false;
        }
        sleep_ms(100);
    }
    
    // Cleanup (though we never reach this in this example)
    adc_cleanup(&adc_config);
    return 0;
}