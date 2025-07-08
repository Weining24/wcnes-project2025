// Copyright (c) 2021 Michael Stoops. All rights reserved.
// Portions copyright (c) 2021 Raspberry Pi (Trading) Ltd.
// 
// Redistribution and use in source and binary forms, with or without modification, are permitted provided that the 
// following conditions are met:
//
// 1. Redistributions of source code must retain the above copyright notice, this list of conditions and the following
//    disclaimer.
// 2. Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the
//    following disclaimer in the documentation and/or other materials provided with the distribution.
// 3. Neither the name of the copyright holder nor the names of its contributors may be used to endorse or promote
//    products derived from this software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, 
// INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE 
// DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR 
// SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, 
// WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE 
// USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
// SPDX-License-Identifier: BSD-3-Clause
//
// Example of an SPI bus master using the PL022 SPI interface

#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/binary_info.h"
#include "hardware/spi.h"

#define BUF_LEN         0x100
// 引脚配置
#define PIN_MISO    18
#define PIN_CS      19  // 实际是IND信号
#define PIN_MODE    20
#define PIN_REQ     21
#define PIN_ACK     22
#define PIN_MOSI    17
#define PIN_SCK     16

// 协议参数
#define MESSAGE_SIZE    128
#define TIMEOUT_MS      200

// SPI 配置
#define SPI_PORT    spi0
#define SPI_BAUDRATE   1000000  // 1 MHz

// 全局变量
uint8_t tx_buffer[MESSAGE_SIZE];
uint8_t rx_buffer[MESSAGE_SIZE];
void init_spi() {
    // 初始化 SPI
    spi_init(SPI_PORT, SPI_BAUDRATE);
    spi_set_format(SPI_PORT, 8, SPI_CPOL_1, SPI_CPHA_1, SPI_MSB_FIRST);
    
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
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_IN);
    
    // 初始状态
    gpio_put(PIN_REQ, 0);  // REQ 高 (未请求)
    gpio_put(PIN_MODE, 0); // MODE 0 (读模式)
    
    // 初始化缓冲区
    memset(tx_buffer, 0, MESSAGE_SIZE);
    memset(rx_buffer, 0, MESSAGE_SIZE);
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

// 执行 SPI 传输 (修正协议)
bool spi_transfer(bool direction) {
    // 1. 设置传输方向
    gpio_put(PIN_MODE, direction);
    
    // 2. 发起请求 (REQ 低->高 上升沿)
    gpio_put(PIN_REQ, 1);  // REQ上升沿触发传输开始
    
    // 3. 等待 ACK 高 (从设备确认)
    if (!wait_for_signal(PIN_ACK, 1, TIMEOUT_MS)) {
        printf("Timeout waiting for ACK high\n");
        gpio_put(PIN_REQ, 0); // 恢复 REQ 低
        return false;
    }
    
    // 4. SPI 数据传输
    spi_write_read_blocking(SPI_PORT, tx_buffer, rx_buffer, MESSAGE_SIZE);
    
    // 5. 结束传输 (REQ 高->低 下降沿)
    gpio_put(PIN_REQ, 0);
    
    // 6. 等待 ACK 低 (从设备确认结束)
    if (!wait_for_signal(PIN_ACK, 0, TIMEOUT_MS)) {
        printf("Timeout waiting for ACK low\n");
        return false;
    }
    
    return true;
}

// 从 MSP430 读取数据
bool read_data() {
    // 检查数据可用指示 (IND)
    if (!gpio_get(PIN_IND)) {
        printf("No data available (IND low)\n");
        return false;
    }
    
    // 设置为读模式
    printf("Reading data...\n");
    return spi_transfer(false); // false = 读模式
}

// 向 MSP430 写入数据
bool write_data(const uint8_t *data, size_t len) {
    if (len > MESSAGE_SIZE) {
        printf("Data too large. Max size: %d\n", MESSAGE_SIZE);
        return false;
    }
    
    // 设置为写模式
    printf("Writing data...\n");
    
    // 准备发送缓冲区
    memset(tx_buffer, 0, MESSAGE_SIZE);
    memcpy(tx_buffer, data, len);
    
    return spi_transfer(true); // true = 写模式
}
// 简单测试协议握手
void test_handshake() {
    printf("\n=== Testing Handshake Protocol ===\n");
    
    // 测试读握手
    printf("Testing read handshake...\n");
    gpio_put(PIN_MODE, 0); // 读模式
    gpio_put(PIN_REQ, 1);  // REQ上升沿
    
    if (wait_for_signal(PIN_ACK, 1, TIMEOUT_MS)) {
        printf("ACK high received\n");
        gpio_put(PIN_REQ, 0); // REQ下降沿
        if (wait_for_signal(PIN_ACK, 0, TIMEOUT_MS)) {
            printf("ACK low received\n");
            printf("Read handshake successful!\n");
        }
    }
    
    // 测试写握手
    printf("\nTesting write handshake...\n");
    gpio_put(PIN_MODE, 1); // 写模式
    gpio_put(PIN_REQ, 1);  // REQ上升沿
    
    if (wait_for_signal(PIN_ACK, 1, TIMEOUT_MS)) {
        printf("ACK high received\n");
        gpio_put(PIN_REQ, 0); // REQ下降沿
        if (wait_for_signal(PIN_ACK, 0, TIMEOUT_MS)) {
            printf("ACK low received\n");
            printf("Write handshake successful!\n");
        }
    }
}
void printbuf(uint8_t buf[], size_t len) {
    size_t i;
    for (i = 0; i < len; ++i) {
        if (i % 16 == 15)
            printf("%02x\n", buf[i]);
        else
            printf("%02x ", buf[i]);
    }

    // append trailing newline if there isn't one
    if (i % 16) {
        putchar('\n');
    }
}

int main() {
    // Enable UART so we can print
    stdio_init_all();
    while(!stdio_usb_connected()) {
        sleep_ms(100);
    }
    printf("Starting SPI master example...\n");
    init_spi();
     // 首先测试握手协议
    test_handshake();
    
    // 测试消息
    uint8_t test_data[] = "Hello MSP430!";
    size_t test_len = sizeof(test_data) - 1; // 不包括结尾的null
    
    // 主循环
    uint32_t counter = 0;
    while (true) {
        printf("\n=== Cycle %lu ===\n", counter);
        
        // 尝试读取数据
        if (read_data()) {
            printf("Read successful. Data: ");
            for (int i = 0; i < 16 && i < MESSAGE_SIZE; i++) {
                printf("%02x ", rx_buffer[i]);
            }
            printf("\n");
        }
        
        // 每5次循环写一次数据
        if (counter % 5 == 0) {
            // 更新测试消息
            snprintf((char*)test_data, sizeof(test_data), "Pico msg %lu", counter/5);
            test_len = strlen((char*)test_data);
            
            if (write_data(test_data, test_len)) {
                printf("Write successful. Sent: %s\n", test_data);
            }
        }
        
        // 等待下次操作
        sleep_ms(500);
        counter++;
    }
    
    return 0;
// #if !defined(spi_default) || !defined(PICO_DEFAULT_SPI_SCK_PIN) || !defined(PICO_DEFAULT_SPI_TX_PIN) || !defined(PICO_DEFAULT_SPI_RX_PIN) || !defined(PICO_DEFAULT_SPI_CSN_PIN)
// #warning spi/spi_master example requires a board with SPI pins
//     puts("Default SPI pins were not defined");
// #else

//     // Enable SPI 0 at 1 MHz and connect to GPIOs
//     init_spi();
//     // Make the SPI pins available to picotool
//     // bi_decl(bi_4pins_with_func(PICO_DEFAULT_SPI_RX_PIN, PICO_DEFAULT_SPI_TX_PIN, PICO_DEFAULT_SPI_SCK_PIN, PICO_DEFAULT_SPI_CSN_PIN, GPIO_FUNC_SPI));

//     uint8_t out_buf[BUF_LEN], in_buf[BUF_LEN];

//     // Initialize output buffer
//     for (size_t i = 0; i < BUF_LEN; ++i) {
//         out_buf[i] = i;
//     }

//     printf("SPI master says: The following buffer will be written to MOSI endlessly:\n");
//     printbuf(out_buf, BUF_LEN);

//     for (size_t i = 0; ; ++i) {
//         // Write the output buffer to MOSI, and at the same time read from MISO.
//         spi_write_read_blocking(spi_default, out_buf, in_buf, BUF_LEN);

//         // Write to stdio whatever came in on the MISO line.
//         printf("SPI master says: read page %d from the MISO line:\n", i);
//         printbuf(in_buf, BUF_LEN);

//         // Sleep for ten seconds so you get a chance to read the output.
//         sleep_ms(10 * 1000);
//     }
// #endif
}
