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
#include "hardware/gpio.h"
#include "hardware/adc.h"

/// \tag::uart_advanced[]

#define UART_ID              uart1
#define BAUD_RATE           115200
#define DATA_BITS                8
#define STOP_BITS                1
#define PARITY    UART_PARITY_NONE

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#define UART_RX_PIN        5
#define UART_BUFFER_SIZE 256
#define UART_END_MARK   '\n'// end of line character
static int  chars_rxed = 0;
/// \end::uart_advanced[]

// SPI pin for MSP430
#define MY_SPI_PORT     spi1
#define SPI_BAUDRATE 5000000  // 5 MHz
#define PIN_MISO          12
#define PIN_MOSI          11
#define PIN_SCK           10

#define PIN_IND     0  // 实际是IND信号
#define PIN_MODE    1
#define PIN_REQ     2
#define PIN_ACK     3



// 协议参数
#define MESSAGE_SIZE    128
#define TIMEOUT_MS      500


//SPI Receiver CC2500
#define RADIO_SPI             spi0
#define RADIO_MISO              16
#define RADIO_MOSI              19
#define RADIO_SCK               18

#define TX_DURATION           5000 // send a packet every 250ms (when changing baud-rate, ensure that the TX delay is larger than the transmission time)
#define RECEIVER              2500 // define the receiver board either 2500 or 1352
#define PIN_TX1                  9
#define PIN_TX2                 22
#define CLOCK_DIV0              20 // larger
#define CLOCK_DIV1              18 // smaller
#define DESIRED_BAUD        100000
#define TWOANTENNAS           true

#define CARRIER_FEQ     2450000000

// 全局变量
uint8_t tx_buffer[MESSAGE_SIZE];
uint8_t rx_buffer[MESSAGE_SIZE];
void my_init_spi() {
    // 初始化 SPI
    spi_init(MY_SPI_PORT, SPI_BAUDRATE);
    spi_set_format(MY_SPI_PORT, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    
    // 设置 SPI 引脚
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    
    // 设置控制引脚
    gpio_init(PIN_REQ);
    gpio_set_dir(PIN_REQ, GPIO_OUT);
    gpio_init(PIN_MODE);
    gpio_set_dir(PIN_MODE, GPIO_OUT);
    gpio_init(PIN_ACK);
    gpio_set_dir(PIN_ACK, GPIO_IN);
    gpio_init(PIN_IND);
    gpio_set_dir(PIN_IND, GPIO_IN);
    
    // 初始状态
    gpio_put(PIN_REQ, 0);  // REQ 高 (未请求)
    gpio_put(PIN_MODE, 0); // MODE 0 (读模式)
    
    // 初始化缓冲区
    memset(tx_buffer, 0, MESSAGE_SIZE);
    memset(rx_buffer, 0, MESSAGE_SIZE);

    // ===== 二进制信息声明 =====
    // SPI1 引脚声明
    bi_decl(bi_3pins_with_func(PIN_MOSI, PIN_MISO, PIN_SCK, GPIO_FUNC_SPI));
    
    // 控制引脚声明
    bi_decl(bi_1pin_with_name(PIN_REQ, "REQ"));
    bi_decl(bi_1pin_with_name(PIN_ACK, "ACK"));
    bi_decl(bi_1pin_with_name(PIN_MODE, "MODE"));
    bi_decl(bi_1pin_with_name(PIN_IND, "IND"));
    
    // 程序信息
    bi_decl(bi_program_name("MSP430-SPI1-Communicator"));
    bi_decl(bi_program_description("SPI1 communication with MSP430FR5969"));
}
// 等待信号变化 (带超时)
bool wait_for_signal(uint pin, bool expected_state, uint timeout_ms) {
    absolute_time_t start_time = get_absolute_time();
    absolute_time_t timeout_time = delayed_by_ms(start_time, timeout_ms);
    
    while (gpio_get(pin) != expected_state) {
        if (absolute_time_diff_us(get_absolute_time(), timeout_time) <= 0) {
            return false; // 超时
        }
        sleep_us(10);
    }
    return true;
}

int8_t test_my_write(const uint8_t *data, size_t len) {
    gpio_put(PIN_REQ, 0);  // REQ reset
    gpio_put(PIN_MODE, 1); // 写模式
    // printf("\n=== Testing My Write Protocol ===\n");
    printf("Backup packet to MSP430 ...\n");
    
    sleep_ms(100); // 等待稳定
    
    gpio_put(PIN_REQ, 1);  // REQ上升沿
    // printf("REQ set high\n");
    //wait for ACK high
    if (wait_for_signal(PIN_ACK, 1, TIMEOUT_MS)) {
        // printf("ACK high received\n");
    } else {
        printf("ERROR: Timeout waiting for ACK high!\n");
        return -1;
    }

    // 4. SPI 数据传输 (只写)
    printf("Beginning SPI data write...\n");
    bool success = spi_is_writable(MY_SPI_PORT);
    // printf("SPI writable? %s\n", success ? "yes" : "no");
    spi_write_blocking(MY_SPI_PORT, data, len);//len 是数据长度 in bytes
    for (size_t i = 0; i < len; ++i) {
        // printf("%c=", rx_data[i]);
        printf("0x%02X ", data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    
    // for (int i = 0; i < len; ++i) {
    //     int ret = spi_write_blocking(MY_SPI_PORT, &data[i], 1);
    //     printf("Wrote byte %d: 0x%02X (ret = %d)\n", i, data[i], ret);
    // }
    gpio_put(PIN_REQ, 0);  
    // printf("REQ set low to write\n");
    printf("Backup completed\n");
    return 0;
}

int8_t test_my_read(uint32_t *data, size_t len) {
    // printf("\n=== Testing My Read Protocol ===\n");
    gpio_put(PIN_REQ, 0);  // REQ reset
    gpio_put(PIN_MODE, 0); // 读模式
    // printf("MODE set to read\n");
    printf("Recover packet from MSP430 ...\n");

    sleep_ms(100); // 等待稳定

    // 发起请求 (REQ 低->高 上升沿)
    gpio_put(PIN_REQ, 1);
    // printf("REQ set high\n");
    
    // 等待 ACK 高 (带超时)
    if (wait_for_signal(PIN_ACK, 1, TIMEOUT_MS)) {
        // printf("ACK high received\n");
    } else {
        printf("ERROR: Timeout waiting for ACK high!\n");
        return -1; // 超时错误
    }
    
    // SPI 数据读取
    // printf("Beginning SPI data read...\n");
    uint32_t *rx_data = malloc(len);
    if (!rx_data) {
        printf("ERROR: Memory allocation failed for rx_data\n");
        return -1; // 内存分配失败
    }   
    memset(rx_data, 0, len); // 清空接收缓冲区
    spi_read_blocking(MY_SPI_PORT, 0, rx_data, len);
    printf("Read %d bytes from SPI:\n", len);
    for (size_t i = 0; i < len; ++i) {
        // printf("%c=", rx_data[i]);
        printf("0x%02X ", rx_data[i]);
        if ((i + 1) % 16 == 0) {
            printf("\n");
        }
    }
    
    // 结束传输 (REQ 高->低 下降沿)
    gpio_put(PIN_REQ, 0);
    // printf("REQ set low to end read\n");
    
    // 等待 ACK 低
    if (wait_for_signal(PIN_ACK, 0, TIMEOUT_MS)) {
        // printf("ACK low detected\n");
    } else {
        printf("ERROR: Timeout waiting for ACK low!\n");
        return -1; // 超时错误
    }
    data = rx_data; // 将读取的数据返回
    free(rx_data); // 释放内存
    printf("Recover packet from MSP430 successfully!\n");
    return 0; // 成功
}

// 初始化所有 GPIO 引脚
void init_gpio() {
    // 控制引脚初始化
    gpio_init(PIN_IND);
    gpio_set_dir(PIN_IND, GPIO_IN);
    gpio_pull_down(PIN_IND);  // 可选：启用下拉
    
    gpio_init(PIN_MODE);
    gpio_set_dir(PIN_MODE, GPIO_OUT);
    gpio_put(PIN_MODE, 0);  // 默认读模式
    
    gpio_init(PIN_REQ);
    gpio_set_dir(PIN_REQ, GPIO_OUT);
    gpio_put(PIN_REQ, 0);  // 初始低电平
    
    gpio_init(PIN_ACK);
    gpio_set_dir(PIN_ACK, GPIO_IN);
    gpio_pull_down(PIN_ACK);  // 可选：启用下拉
    
    // SPI 引脚初始化（即使设置了SPI功能，也建议显式初始化）
    gpio_init(PIN_SCK);
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    
    gpio_init(PIN_MOSI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    gpio_init(PIN_MISO);
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);
    
    printf("GPIO initialized\n");
}
// RX interrupt handler
int on_uart_rx() {
    uint16_t rx_index = 0;
    uint8_t rx_buffer_0[UART_BUFFER_SIZE] = {0};
    uint8_t rx_buffer_1[UART_BUFFER_SIZE] = {0};
    while (uart_is_readable(UART_ID)) {
        // printf("UART data received...\n");
        int8_t ch = uart_getc(UART_ID);
        // printf("Received char: %c\n", ch);
        if(rx_index < UART_BUFFER_SIZE - 1) {
            rx_buffer_0[rx_index++] = ch;
        } else {
            rx_index = 0; // reset index if buffer is full
        }

        if(ch == UART_END_MARK) {
            // We have a complete line, so print it out.
            rx_buffer_0[rx_index] = '\0'; // null-terminate the string
            // printf("Received: %s\n", rx_buffer);
            rx_index = 0; // reset index for next line
        }
    }
    while (uart_is_readable(UART_ID)) {
        // printf("UART data received...\n");
        int8_t ch = uart_getc(UART_ID);
        // printf("Received char: %c\n", ch);
        if(rx_index < UART_BUFFER_SIZE - 1) {
            rx_buffer_1[rx_index++] = ch;
        } else {
            rx_index = 0; // reset index if buffer is full
        }

        if(ch == UART_END_MARK) {
            // We have a complete line, so print it out.
            rx_buffer_1[rx_index] = '\0'; // null-terminate the string
            // printf("Received: %s\n", rx_buffer);
            rx_index = 0; // reset index for next line
        }
    }

    printf("Received: %s\n", rx_buffer_1);
    //Convert the received data to an integer
    int received_value = 0;
    if (sscanf((const char *)rx_buffer_1, "%d", &received_value) == 1) {
        printf("Received integer value: %d\n", received_value);
        return received_value;
    } else {
        printf("Failed to parse integer from received data.\n");
        return -1; // or some error code
    }
}

//Get voltage from ADC
float get_voltage() {
    const float conversion_factor = 3.3f / (1 << 12);
    uint16_t result = adc_read();
    float voltage = result * conversion_factor;
    printf("ADC Voltage: %.2f V\n", voltage);
    return voltage;
}

int main() {
    /* setup SPI */
    stdio_init_all();
    while(!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Starting carrier-receiver baseband example...\n");

    my_init_spi(); // Initialize SPI with custom function
    spi_init(RADIO_SPI, 5 * 1000000); // SPI0 at 5MHz.
    gpio_set_function(RADIO_SCK, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MOSI, GPIO_FUNC_SPI);
    gpio_set_function(RADIO_MISO, GPIO_FUNC_SPI);

    // Make the SPI pins available to picotool
    bi_decl(bi_3pins_with_func(RADIO_MOSI, RADIO_MISO, RADIO_SCK, GPIO_FUNC_SPI));
    bi_decl(bi_3pins_with_func(PIN_MOSI, PIN_MISO, PIN_SCK, GPIO_FUNC_SPI));

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

    sleep_ms(2000);

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

    // Set up our UART with a baud rate.to access RSSI frim CC2640R2
    uart_init(UART_ID, 115200);
    gpio_set_function(UART_RX_PIN, UART_FUNCSEL_NUM(UART_ID, UART_RX_PIN));
    // int __unused actual = uart_set_baudrate(UART_ID, BAUD_RATE);
    // uart_set_hw_flow(UART_ID, false, false);
    // uart_set_format(UART_ID, DATA_BITS, STOP_BITS, PARITY);
    // int UART_IRQ = UART1_IRQ;

    // // And set up and enable the interrupt handlers
    // irq_set_exclusive_handler(UART_IRQ, on_uart_rx);
    // irq_set_enabled(UART_IRQ, true);
    // uart_set_irq_enables(UART_ID, true, false);
    // irq_set_priority(UART_IRQ, 0);

    uint8_t test_data[] = "Hello MSP430!";
    size_t test_len = sizeof(test_data) - 1; // 不包括结尾的null

    uint32_t counter = 0;
    float voltage = 0.0;

    init_gpio();
    // while(1){
    //     test_my_write(test_data, test_len);
    //     sleep_ms(5000); // 等待1秒
    //     test_my_read(test_len);
    //     // test_my_read();
    //     // test_write_only();
    //     sleep_ms(5000); // 等待5秒
    //     test_my_read(test_len);
    //     sleep_ms(5000); // 等待5秒
    // }

    // while(1){
    //     printf("Waiting for UART data...\n");
    //     sleep_ms(1000); // wait for 1 second before checking again
    //     // on_uart_rx(); // wait for 1 second before checking again
    //     // printf("Done...\n");
    // }
    adc_init();
    adc_gpio_init(26); // Initialize GPIO 26 for ADC
    adc_select_input(0); // Select ADC input 0 (GPIO 26)
    evt = init_evt; // Initialize event to no_evt
    static bool MSP430_flag = false; // Flag to indicate if MSP430 data is available
    static uint16_t MPS430_counter = 0; // Counter for MSP430 data
    while (true) {
        // evt = get_event();
        switch(evt){
            case rx_assert_evt:
                // evt = get_event(); // get next event
                // if (evt != rx_assert_evt) {
                //     printf("Unexpected event after rx_assert_evt: %d\n", evt);
                //     break;
                // }
                // started receiving
                rx_ready = false;
                // evt = rx_deassert_evt; // reset event to rx_deassert_evt
            break;
            case rx_deassert_evt:
                // finished receiving
                // evt = get_event(); // get next event
                // if(evt != rx_deassert_evt) {
                //     printf("Unexpected event after rx_deassert_evt: %d\n", evt);
                //     evt = init_evt; // reset event to init_evt
                //     break;
                // }
                time_us = to_us_since_boot(get_absolute_time());
                status = readPacket(rx_buffer);
                printPacket(rx_buffer,status,time_us);
                RX_start_listen();
                rx_ready = true;
                // evt = init_evt; // reset event to init_evt
            break;
            case sleep_evt:
                voltage = get_voltage();
                if (voltage < 2.0) {
                    printf("Voltage is low: %.2f V, keep sleep.\n", voltage);
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    evt = sleep_evt; // set event to backup_evt
                    break; // continue to the next iteration
                } else {
                    printf("Voltage is sufficient: %.2f V, weak up!\n", voltage);
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    evt = init_evt;
                    break; // continue to the next iteration
                }
            case init_evt:
                // initialize the system
                printf("Initializing system...\n");
                voltage = get_voltage();
                if (voltage < 2.0) {
                    printf("Voltage is low: %.2f V, back to sleep.\n", voltage);
                    evt = sleep_evt; // set event to sleep_evt
                    sleep_ms(5000); // wait for 1 second before checking again
                    break; // continue to the next iteration
                }
                if (MSP430_flag) {
                    if (test_my_read(buffer, buffer_size(PAYLOADSIZE, HEADER_LEN)) == -1) {
                        printf("No data stored in MSP430 or read failed, trying to sense new data...\n");
                        sleep_ms(5000); // wait for 5 seconds before checking again
                        MSP430_flag = false; // reset the flag
                        evt = sense_evt; // set event to sense_evt
                        break; // continue to the next iteration
                    } else {
                        printf("Data recovered from MSP430 successfully.\n");
                        MSP430_counter--; // decrement the MSP430 counter
                        if (MSP430_counter == 0) {
                            MSP430_flag = false; // reset the flag if no more data
                        }
                        evt = no_evt; // transmission event
                        sleep_ms(5000); // wait for 5 seconds before checking again
                        break;
                    }
                } else {
                    printf("No MSP430 data available, generating new data...\n");
                    evt = sense_evt; // set event to sense_evt
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    break; // continue to the next iteration
                }
                 // read data from MSP430
            break;
            case sense_evt:
                /* generate new data */
                //Pretend to generate data for transmission
                printf("Generating new data for transmission...\n");
                generate_data(tx_payload_buffer, PAYLOADSIZE, true);

                /* add header (10 byte) to packet */
                add_header(&message[0], seq, header_tmplate);
                /* add payload to packet */
                memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);

                /* casting for 32-bit fifo */
                for (uint8_t i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
                    buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
                }
                sleep_ms(5000); // wait for 5 seconds before checking again
                evt = RSSI_evt; // set event to RSSI_evt
            break;
            case backup_evt:
                // backup the current packet
                printf("Backing up current packet...\n");
                if (test_my_write(buffer, buffer_size(PAYLOADSIZE, HEADER_LEN)) == 0) {
                    printf("Data backed up to MSP430 successfully.\n");
                    MSP430_flag = true; // set the flag to indicate data is available
                    MSP430_counter++; // increment the MSP430 counter
                    evt = sleep_evt; // set event to sleep_evt
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    break; // continue to the next iteration
                } else {
                    printf("Failed to backup data to MSP430. Check the connection.\n");
                    evt = sleep_evt; // set event to sleep_evt
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    break; // continue to the next iteration
                }
            break;
            case recover_evt:
                printf("Recovering data from MSP430...\n");
                if (test_my_read(buffer, buffer_size(PAYLOADSIZE, HEADER_LEN)) == -1) {
                    printf("No data stored in MSP430 or read failed, trying to sense new data...\n");
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    MSP430_flag = false; // reset the flag
                    evt = sense_evt; // set event to sense_evt
                    break; // continue to the next iteration
                } else {    
                    printf("Data recovered from MSP430 successfully.\n");
                    MSP430_counter--; // decrement the MSP430 counter
                    if (MSP430_counter == 0) {
                        MSP430_flag = false; // reset the flag if no more data
                    }
                    evt = no_evt; // transmission event
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    break;
                }
            break;
            case voltagecheck_evt:
                //Check the voltage from ADC
                printf("Checking voltage...\n");
                voltage = get_voltage();
                if (voltage < 2.0) {
                    printf("Voltage is low: %.2f V, consider backup the current packet.\n", voltage);
                    evt = backup_evt; // set event to backup_evt
                    break; // continue to the next iteration
                } else {
                    printf("Voltage is sufficient: %.2f V\n", voltage);
                    evt = RSSI_evt;
                    break; // continue to the next iteration
                }
            break;
            case RSSI_evt:
                // This event is used to read RSSI from CC2640R2
                // Ask for RSSI from CC2640R2 regularly
                voltage = get_voltage();
                if (voltage < 2.0) {
                    printf("Voltage is low: %.2f V, consider backup the current packet.\n", voltage);
                    evt = backup_evt; // set event to backup_evt
                    break; // continue to the next iteration
                }
                printf("Checking RSSI...\n");
                int Current_RSSI = 0;
                Current_RSSI = on_uart_rx(); // Call the function to read data from CC2640R2
                if (Current_RSSI != -1) {
                    printf("Current RSSI: %d\n", Current_RSSI);
                    // Process the received data if needed

                    if (Current_RSSI < -50) {
                        printf("Signal is weak, consider moving closer to the transmitter.\n");
                        evt = RSSI_evt; // reset event to no_evt
                        sleep_ms(5000); // wait for 1 second before checking again
                        break; // continue to the next iteration
                    } else if (Current_RSSI >= -50) {
                        printf("Signal is strong, good connection!\n");
                        evt = no_evt; // reset event to transmission event
                        sleep_ms(5000); // wait for 5 seconds before checking again
                        break;
                    }
                } else {
                    printf("No data received from CC2640R2, try again.\n");
                    // sleep_ms(1000); // wait for 1 second before checking again
                    evt = RSSI_evt; // reset event to no_evt
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    break; // continue to the next iteration
                }

            break;
            case no_evt: // actual transmission event
                // backscatter new packet if receiver is listening
                // evt = get_event(); // get next event
                // if (evt != no_evt) {
                //     printf("Unexpected event: %d\n", evt);
                //     break;
                // }
                // printf("current event: %d\n", evt);
                // printf("rx_ready: %d\n", rx_ready);
                // if (rx_ready)
                {

                    /* generate new data */
                    // generate_data(tx_payload_buffer, PAYLOADSIZE, true);

                    // /* add header (10 byte) to packet */
                    // add_header(&message[0], seq, header_tmplate);
                    // /* add payload to packet */
                    // memcpy(&message[HEADER_LEN], tx_payload_buffer, PAYLOADSIZE);

                    // /* casting for 32-bit fifo */
                    // for (uint8_t i=0; i < buffer_size(PAYLOADSIZE, HEADER_LEN); i++) {
                    //     buffer[i] = ((uint32_t) message[4*i+3]) | (((uint32_t) message[4*i+2]) << 8) | (((uint32_t) message[4*i+1]) << 16) | (((uint32_t)message[4*i]) << 24);
                    // }
                    /* put the data to FIFO (start backscattering) */
                    // startCarrier();
                    sleep_ms(1); // wait for carrier to start
                    backscatter_send(pio,sm,buffer,buffer_size(PAYLOADSIZE, HEADER_LEN));
                    sleep_ms(ceil((((double) buffer_size(PAYLOADSIZE, HEADER_LEN))*8000.0)/((double) DESIRED_BAUD))+3); // wait transmission duration (+3ms)
                    // stopCarrier();
                    /* increase seq number*/ 
                    seq++;
                    printf("Backscattered packet with seq: %d\n", seq);
                }
                sleep_ms(TX_DURATION);
                // evt = rx_assert_evt; // set event to rx_assert_evt
                if (MSP430_flag == false) {
                    printf("No MSP430 data available, generating new data...\n");
                    evt = sense_evt; // set event to sense_evt
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    break;
                } else {
                    printf("Try to reover data from MSP430...\n");
                    evt = recover_evt; // set event to recover_evt
                    sleep_ms(5000); // wait for 5 seconds before checking again
                    break;
                }
            break;
        }
        sleep_ms(1);
    }

    /* stop carrier and receiver - never reached */
    RX_stop_listen();
    stopCarrier();
}
