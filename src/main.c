#include "buddy1/sd_card.h"
#include "buddy2/signal_analyzer.h"
#include "buddy3/protocol_analyzer.h"
#include "buddy3/signal_generator.h"


int main() {
    stdio_init_all();
    sleep_ms(2000);  // Give time for USB serial to initialize
    
    printf("Protocol Analyzer Starting...\n");

    // Initialize protocol analyzer with all pins
    protocol_analyzer_init(UART_RX_PIN, I2C_SCL_PIN, I2C_SDA_PIN, 
                         SPI_SCK_PIN, SPI_MOSI_PIN, SPI_MISO_PIN);

    while(1) {
        printf("\nAnalyzing signals...\n");
        protocol_result_t result = analyze_protocol(1000);
        display_protocol_results(&result);
        
        sleep_ms(1000);  // Wait before next analysis
    }
    
    return 0;
}