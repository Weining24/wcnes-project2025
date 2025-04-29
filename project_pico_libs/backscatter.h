/*
 * Tobias Mages & Wenqing Yan
 * Course: Wireless Communication and Networked Embedded Systems, Project VT2023
 * Backscatter PIO
 * 29-March-2023
 */

#if !PICO_NO_HARDWARE
#include "hardware/pio.h"
#endif

#include <stdio.h>
#include <math.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/clocks.h"

#define CLKFREQ 125
#ifndef MINMAX
#define MINMAX
#define max(x, y) (((x) > (y)) ? (x) : (y))
#define min(x, y) (((x) < (y)) ? (x) : (y))
#endif
#define abs(x) (((x) > (0)) ? (x) : (-x))

#define ASM_SET_PINS  0xE000
#define ASM_OUT       0x6000
#define ASM_JMP       0x0000 // JMP
#define ASM_JMP_NOTX  0x0020 // JMP !x
#define ASM_JMP_XMM   0x0040 // JMP x--
#define ASM_MOV       0xA000
#define ASM_X_REG     0x0001
#define ASM_Y_REG     0x0002
#define ASM_ISR_REG   0x0006
#define ASM_WAIT_PIN  0x20A0 // WAIT 1 pin 0

#ifndef PIO_BACKSCATTER
#define PIO_BACKSCATTER
struct backscatter_config {
  uint32_t baudrate;
  uint32_t center_offset;
  uint32_t deviation;
  uint32_t minRxBw;
};

/* Type to describe a square wave with phase delay.
   Each period can be viewed as split into 3 parts,
   each lasting cycles[i] cycles with value values[i]
*/
typedef struct square_wave {
  uint8_t values[3]; // e.g. {1, 0, 1} for 90 degree phase
  uint16_t cycles[3]; // e.g. {7, 14, 7} for 90 degree phase
} square_wave_t;
#endif

// ----------- //
// backscatter //
// ----------- //

// how many instructions are needed to create this delay?
uint8_t instructionCount(uint16_t delay, uint16_t max_delay);

// repeat the instruction until the desired delay has past
int16_t repeat(uint16_t* instructionBuffer, int16_t delay, uint32_t asm_instr, uint8_t *length, uint16_t max_delay);

// compute the number of delay cycles corresponding to a given phase shift at the center frequency
uint32_t phase_shift_to_delay_cycles(uint16_t phase_shift, uint16_t d0, uint16_t d1);

bool generatePIOprogram(uint16_t d0,uint16_t d1, uint32_t baud, uint16_t* instructionBuffer, struct pio_program *backscatter_program);

/* based on d0/d1/baud, the modulation parameters will be computed and returned in the struct backscatter_config */
void backscatter_program_init(PIO pio, uint8_t *state_machines, uint16_t *pins, uint16_t pin_start_tx, uint8_t num_antennae, uint16_t d0, uint16_t d1, uint32_t baud, struct backscatter_config *config, uint16_t *instructionBuffer);

void backscatter_send(PIO pio, uint8_t *state_machines, uint8_t *dma_channels, uint16_t pin_start_tx, uint32_t *phase_delay_cycles, uint8_t num_antennae, uint32_t *message, uint32_t message_length);
