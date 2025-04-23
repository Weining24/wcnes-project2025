#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "backscatter.h"         // From project_pico_libs
#include "packet_generation.h"   // From project_pico_libs

#define PIN_TX1         6
#define PIN_TX2         27
#define CLOCK_DIV0      20
#define CLOCK_DIV1      18
#define DESIRED_BAUD    100000
#define TWOANTENNAS     true
#define TX_INTERVAL_MS  250

int main() {
    stdio_init_all();
    sleep_ms(3000); // Give time for serial connection or debugging

    printf("Backscatter Pico starting...\n");

    // Initialize PIO for backscatter function
    PIO pio = pio0;
    uint sm = 0;
    struct backscatter_config backscatter_conf;
    uint16_t instructionBuffer[32] = {0};

    backscatter_program_init(pio, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, 
                               &backscatter_conf, instructionBuffer, TWOANTENNAS);

    // Create buffers and set header for TI CC1352 receiver
    static uint8_t message[buffer_size(PAYLOADSIZE+2, HEADER_LEN)*4] = {0};
    static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0};
    static uint8_t seq = 0;
    uint8_t *header_template = packet_hdr_template(1352);
    uint8_t tx_payload_buffer[PAYLOADSIZE];

    int count = 0;
    while (count < 256) {
        generate_data(tx_payload_buffer, PAYLOADSIZE, true);

        // Construct packet: header plus payload
        add_header(&message[0], seq, header_template);
        memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);

        // Convert the packet into a 32-bit array for the PIO
        for (uint8_t i = 0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
            buffer[i] = ((uint32_t) message[4*i+3]) |
                        (((uint32_t) message[4*i+2]) << 8) |
                        (((uint32_t) message[4*i+1]) << 16) |
                        (((uint32_t) message[4*i]) << 24);
        }

        // Backscatter packet transmit
        printf("TX: Sending packet %d\n", seq);
        backscatter_send(pio, sm, buffer, buffer_size(PAYLOADSIZE, HEADER_LEN));
        sleep_ms(ceil((((double) buffer_size(PAYLOADSIZE, HEADER_LEN)) * 8000.0) / ((double)DESIRED_BAUD)) + 3);
        seq++;
        sleep_ms(TX_INTERVAL_MS);
        
        count++;
        if(count >= 255){
            printf("MAX count REACHED: %i\n EXITING...", count);
            break;
        }

    }
    
    return 0;
}