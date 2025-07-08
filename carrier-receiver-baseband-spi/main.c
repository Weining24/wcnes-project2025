/**
 * Tobias Mages & Wenqing Yan
 * Backscatter PIO
 * 02-March-2023
 *
 * See the sub-projects ... for further information:
 *  - baseband
 *  - carrier-CC2500
 *  - receiver-CC2500
 *
 */

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"

#include "pico/util/queue.h"
#include "pico/binary_info.h"
#include "pico/util/datetime.h"
#include "hardware/spi.h"

#include "hardware/pio.h"
#include "hardware/clocks.h"
#include "backscatter.h"
#include "carrier_CC2500.h"
#include "receiver_CC2500.h"
#include "packet_generation.h"

#include "hardware/uart.h"
#include "hardware/irq.h"

/// \tag::uart_advanced[]

#define UART_ID uart1
#define BAUD_RATE 115200
#define DATA_BITS 8
#define STOP_BITS 1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_RX_PIN 5
#define UART_BUFFER_SIZE 256
#define UART_END_MARK '\n' // end of line character
static int chars_rxed = 0;
/// \end::uart_advanced[]


#define RADIO_SPI             spi0
#define RADIO_MISO              16
#define RADIO_MOSI              19
#define RADIO_SCK               18

#define TX_DURATION            250 // send a packet every 250ms (when changing baud-rate, ensure that the TX delay is larger than the transmission time)
#define RECEIVER              1352 // define the receiver board either 2500 or 1352
#define PIN_TX1                  9
#define PIN_TX2                 22
#define CLOCK_DIV0              20 // larger
#define CLOCK_DIV1              18 // smaller
#define DESIRED_BAUD        100000
#define TWOANTENNAS          true

#define CARRIER_FEQ     2450000000

// RX interrupt handler
void on_uart_rx() {
    uint16_t rx_index = 0;
    uint8_t rx_buffer[UART_BUFFER_SIZE] = {0};
    while (uart_is_readable(UART_ID)) {
        // printf("UART data received...\n");
        int8_t ch = uart_getc(UART_ID);
        // printf("Received char: %c\n", ch);
        if(rx_index < UART_BUFFER_SIZE - 1) {
            rx_buffer[rx_index++] = ch;
        } else {
            rx_index = 0; // reset index if buffer is full
        }

        if(ch == UART_END_MARK) {
            // We have a complete line, so print it out.
            rx_buffer[rx_index] = '\0'; // null-terminate the string
            // printf("Received: %s\n", rx_buffer);
            rx_index = 0; // reset index for next line
        }
        // Can we send it back?
        // if (uart_is_writable(UART_ID)) {
        //     // Change it slightly first!
        //     ch++;
        //     uart_putc(UART_ID, ch);
        // }
    }
    printf("Received: %s\n", rx_buffer);
}

int main() {
    /* setup SPI */
    stdio_init_all();
    while(!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Starting carrier-receiver baseband example...\n");

    spi_init(RADIO_SPI, 5 * 1000000); // SPI0 at 5MHz.
    gpio_set_function(RADIO_SCK, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MISO, GPIO_FUNC_SPI);

    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(RADIO_MOSI, RADIO_MISO, RADIO_SCK, GPIO_FUNC_SPI));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(RX_CSN);
    gpio_set_dir(RX_CSN, GPIO_OUT);
    gpio_put(RX_CSN, 1);
    bi_decl(bi_1pin_with_name(RX_CSN, "SPI Receiver CS"));

    // Chip select is active-low, so we'll initialise it to a driven-high state
    gpio_init(CARRIER_CSN);
    gpio_set_dir(CARRIER_CSN, GPIO_OUT);
    gpio_put(CARRIER_CSN, 1);
    bi_decl(bi_1pin_with_name(CARRIER_CSN, "SPI Carrier CS"));







    sleep_ms(5000);

    /* setup backscatter state machine */
    PIO pio = pio0;
    uint sm = 0;
    struct backscatter_config backscatter_conf;
    uint16_t instructionBuffer[32] = {0}; // maximal instruction size: 32
    backscatter_program_init(pio, sm, PIN_TX1, PIN_TX2, CLOCK_DIV0, CLOCK_DIV1, DESIRED_BAUD, &backscatter_conf, instructionBuffer, TWOANTENNAS);

    static uint8_t message[buffer_size(PAYLOADSIZE+2, HEADER_LEN)*4] = {0};  // include 10 header bytes
    static uint32_t buffer[buffer_size(PAYLOADSIZE, HEADER_LEN)] = {0}; // initialize the buffer
    static uint8_t seq = 0;
    uint8_t *header_tmplate = packet_hdr_template(RECEIVER);
    uint8_t tx_payload_buffer[PAYLOADSIZE];

    /* Setup carrier */
    printf("\nConfiguring one CC2500 as carrier generator:\n");
    setupCarrier();
    set_frecuency_tx(CARRIER_FEQ);
    sleep_ms(1);

    /* Start Receiver */
    printf("\nConfiguring one CC2500 to approximate the obtained radio settings:\n");
    event_t evt = no_evt;
    Packet_status status;
    uint8_t rx_buffer[RX_BUFFER_SIZE];
    uint64_t time_us;
    setupReceiver();
    set_frecuency_rx(CARRIER_FEQ + backscatter_conf.center_offset);
    set_frequency_deviation_rx(backscatter_conf.deviation);
    set_datarate_rx(backscatter_conf.baudrate);
    set_filter_bandwidth_rx(backscatter_conf.minRxBw);
    sleep_ms(1);
    RX_start_listen();
    printf("started listening\n");
    bool rx_ready = true;
    printf("TX baudrate: %u\n", backscatter_conf.baudrate);
    printf("Freq offset: %d\n", backscatter_conf.center_offset);
    printf("Deviation  : %d\n", backscatter_conf.deviation);
    /* loop */

        // Set up our UART with a basic baud rate.
    uart_init(UART_ID, 115200);
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));
    // int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);
    // uart_set_hw_flow(UART_ID, false, false);
    // uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    int UART_IRQ = UART1_IRQ;

    // // And set up and enable the interrupt handlers
    irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    irq_set_enabled(UART_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
    irq_set_priority(UART_IRQ, 0);
    while(1){
        printf("Waiting for UART data...\n");
        sleep_ms(1000); // wait for 1 second before checking again
        // on_uart_rx(); // wait for 1 second before checking again
        // printf("Done...\n");
    }

    while (true) {
        evt = get_event();
        switch(evt){
            case rx_assert_evt:
                // started receiving
                rx_ready = false;
            break;
            case rx_deassert_evt:
                // finished receiving
                time_us = to_us_since_boot(get_absolute_time());
                status = readPacket(rx_buffer);
                printPacket(rx_buffer,status,time_us);
                RX_start_listen();
                rx_ready = true;
            break;
            case no_evt:
                // backscatter new packet if receiver is listening
                if (rx_ready){
                    /* generate new data */
                    generate_data(tx_payload_buffer, PAYLOADSIZE, true);

                    /* add header (10 byte) to packet */
                    add_header(&message[0], seq, header_tmplate);
                    /* add payload to packet */
                    memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);

                    /* casting for 32-bit fifo */
                    for (uint8_t i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
                        buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
                    }
                    /* put the data to FIFO (start backscattering) */
                    startCarrier();
                    sleep_ms(1); // wait for carrier to start
                    backscatter_send(pio,sm,buffer,buffer_size(PAYLOADSIZE, HEADER_LEN));
                    sleep_ms(ceil((((double) buffer_size(PAYLOADSIZE, HEADER_LEN))*8000.0)/((double) DESIRED_BAUD))+3); // wait transmission duration (+3ms)
                    stopCarrier();
                    /* increase seq number*/ 
                    seq++;
                }
                sleep_ms(TX_DURATION);
            break;
        }
        sleep_ms(1);
    }

    /* stop carrier and receiver - never reached */
    RX_stop_listen();
    stopCarrier();
}
