/**
 * Tobias Mages & Wenqing Yan
 * 
 * support functions to generate payload data and function
 * 
 */

#include <stdio.h>
#include <string.h>
#include <math.h>
#include "pico/stdlib.h"
#include "packet_generation.h"

#define DEFAULT_SEED 0xABCD
uint32_t seed = DEFAULT_SEED;

uint8_t packet_hdr_2500[HEADER_LEN] = {0xaa, 0xaa, 0xaa, 0xaa, 0xd3, 0x91, 0xd3, 0x91, 0x00, 0x00};    // CC2500, the last two byte one for the payload length. and another is seq number
uint8_t packet_hdr_1352[HEADER_LEN] = {0xaa, 0xaa, 0xaa, 0xaa, 0x93, 0x0b, 0x51, 0xde, 0x00, 0x00};    // CC1352P7, the last two byte one for the payload length. and another is seq number

/*
 * obtain the packet header template for the corresponding radio
 * buffer: array of size HEADER_LEN 
 * receiver: radio number (2500 or 1352)
 */
uint8_t *packet_hdr_template(uint16_t receiver){
    if(receiver == 2500){
        return packet_hdr_2500;
    }else{
        return packet_hdr_1352;
    }
}

/* 
 * generate of a uniform random number.
 */
uint32_t rnd() {
    const uint32_t A1 = 1664525;
    const uint32_t C1 = 1013904223;
    const uint32_t RAND_MAX1 = 0xFFFFFFFF;
    seed = ((seed * A1 + C1) & RAND_MAX1);
    return seed;
}

/* 
 * generate compressible payload sample
 * file_position provides the index of the next data byte (increments by 2 each time the function is called)
 */
uint16_t file_position = 0;
uint16_t generate_sample(){
    if (file_position == 0) {
        seed = DEFAULT_SEED; /* reset seed when exceeding uint16_t max */
    }
    file_position = file_position + 2;
    double two_pi = 2.0 * M_PI;
    double u1, u2;
    u1 = ((double) rnd())/ ((double) 0xFFFFFFFF);
    u2 = ((double) rnd())/((double) 0xFFFFFFFF);
    double tmp = ((double) 0x7FF) * sqrt(-2.0 * log(u1));
    return max(0.0,min(((double) 0x3FFFFF),tmp * cos(two_pi * u2) + ((double) 0x1FFF)));
}

/*
 * fill packet with 16-bit samples
 * include_index: shall the file index be included at the first two byte?
 * length: the length of the buffer which can be filled with data
*/
void generate_data(uint8_t *buffer, uint8_t length, bool include_index) {
    static const uint8_t encoded_payloads[3][32] = {
        { 0x69, 0x70, 0x55, 0x69, 0x5a, 0x70, 0x70, 0x7f, 0x00, 0x0f, 0x16, 0x19, 0x25, 0x2a, 0x33, 0x3c, 0x43, 0x4c, 0x55, 0x5a, 0x66, 0x69, 0x70, 0x7f, 0x0f, 0x00, 0x19, 0x16, 0x2a, 0x25, 0x3c, 0x33 },
        { 0x0f, 0x0f, 0x16, 0x16, 0x19, 0x19, 0x25, 0x25, 0x2a, 0x2a, 0x33, 0x33, 0x3c, 0x3c, 0x43, 0x43, 0x4c, 0x4c, 0x55, 0x55, 0x5a, 0x5a, 0x66, 0x66, 0x69, 0x69, 0x70, 0x70, 0x7f, 0x7f, 0x00, 0x00},
        { 0x0f, 0x16, 0x19, 0x25, 0x2a, 0x33, 0x3c, 0x43, 0x4c, 0x55, 0x5a, 0x66, 0x69, 0x70, 0x7f, 0x00, 0x16, 0x0f, 0x25, 0x19, 0x33, 0x2a, 0x43, 0x3c, 0x55, 0x4c, 0x66, 0x5a, 0x70, 0x69, 0x00, 0x7f}
    };
    static int payload_index = 0;
    int num_payloads = sizeof(encoded_payloads) / sizeof(encoded_payloads[0]);

    if(length % 2 != 0){
        printf("WARNING: generate_data has been used with an odd length.");
    }

    uint8_t data_start = 0;
    if(include_index){
        buffer[0] = (uint8_t) (file_position >> 8);
        buffer[1] = (uint8_t) (file_position & 0x00FF);
        data_start = 2;
    }

    // Select the current encoded payload
    const uint8_t* static_payload = encoded_payloads[payload_index];

    // Copy encoded payload to buffer (truncate or repeat as needed)
    uint8_t payload_len = length - data_start;
    if (payload_len > 32) {
        payload_len = 32;
    }
    memcpy(&buffer[data_start], static_payload, payload_len);

    // If the payload is shorter than requested, pad the rest with zeros
    for (uint8_t i = data_start + payload_len; i < length; i++) {
        buffer[i] = 0;
    }

    // Move to the next payload for next call
    payload_index = (payload_index + 1) % num_payloads;
}

/* including a header to the packet:
 * - 8B header sequence
 * - 1B payload length
 * - 1B sequence number
 *
 * packet: buffer to be updated with the header
 * seq: sequence number of the packet
 * header_template: obtained using packet_hdr_template()
 */
void add_header(uint8_t *packet, uint8_t seq, uint8_t *header_template) {
    /* fill in the header sequence*/
    for(int loop = 0; loop < HEADER_LEN-2; loop++) {
        packet[loop] = header_template[loop];
        }
    /* add the payload length*/
    packet[HEADER_LEN-2] = 1 + PAYLOADSIZE; // The packet length is defined as the payload data, excluding the length byte and the optional CRC. (cc2500 data sheet, p. 30)
    /* add the packet as sequence number. */
    packet[HEADER_LEN-1] = seq;
}

