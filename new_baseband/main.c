/**
 * Backscatter PIO - Minimal Backscatter Transmitter
 * Modified for Firefly carrier and CC1352 receiver (no CC2500)
 */

 #include <stdio.h>
 #include <math.h>
 #include <string.h>
 #include "pico/stdlib.h"
 
 #include "pico/util/queue.h"
 #include "pico/binary_info.h"
 #include "pico/util/datetime.h"
 #include "hardware/pio.h"
 #include "hardware/clocks.h"
 
 #include "backscatter.h"
 #include "packet_generation.h"
 
 // #include "carrier_CC2500.h"
 // #include "receiver_CC2500.h"
 
 #define TX_DURATION            250 // send a packet every 250ms
 #define RECEIVER              1352 // still used for header format
 #define PIN_TX1                  6
 #define PIN_TX2                 27
 #define CLOCK_DIV0              20
 #define CLOCK_DIV1              18
 #define DESIRED_BAUD        100000
 #define TWOANTENNAS          true
 
 // #define CARRIER_FEQ     2450000000
 
 int main() {
     stdio_init_all();
     sleep_ms(5000); // Give time for USB to initialize
 
     /* setup backscatter state machine */
     PIO pio = pio0;
     uint sm = 0;
     struct backscatter_config backscatter_conf;
     uint16_t instructionBuffer[32] = {0}; // Max instruction size = 32
     backscatter_program_init(pio, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS);
 
     static uint8_t message[buffer_size(PAYLOADSIZE+2, HEADER_LEN)*4] = {0};  // includes header
     static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // data buffer
     static uint8_t seq = 0;
     uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
     uint8_t tx_payload_buffer[PAYLOADSIZE];
 
     /* loop */
     while (true) {
        generate_data(tx_payload_buffer, PAYLOADSIZE, true);
        add_header(&message[0], seq, header_tmplate);
        memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);
    
        printf("SENDING PACKET #%u\n", seq);
        printf("Total packet length: %u bytes\n", HEADER_LEN + PAYLOADSIZE);
    
        printf("Header: ");
        for (int i = 0; i < HEADER_LEN; i++) {
            printf("%02X ", message[i]);
        }
        printf("\n");
    
        printf("Payload: ");
        for (int i = 0; i < (PAYLOADSIZE > 16 ? 16 : PAYLOADSIZE); i++) {
            printf("%02X ", tx_payload_buffer[i]);
        }
        if (PAYLOADSIZE > 16) printf("...");
        printf("\n");
    
        for (uint8_t i = 0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
            buffer[i] = ((uint32_t) message[4*i+3]) |
                        (((uint32_t) message[4*i+2]) << 8) |
                        (((uint32_t) message[4*i+1]) << 16) |
                        (((uint32_t) message[4*i]) << 24);
        }
    
        backscatter_send(pio, sm, buffer, buffer_size(PAYLOADSIZE, HEADER_LEN));
        sleep_ms(ceil((((double) buffer_size(PAYLOADSIZE, HEADER_LEN)) * 8000.0) / ((double) DESIRED_BAUD)) + 3);
        seq++;
        sleep_ms(TX_DURATION);
    }
    
     return 0;
 }
 