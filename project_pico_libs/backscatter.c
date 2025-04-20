/*
 * Tobias Mages & Wenqing Yan
 * Course: Wireless Communication and Networked Embedded Systems, Project VT2023
 * Backscatter PIO
 * 29-March-2023
 */

#include "backscatter.h"
#include <math.h>

// repeat the instruction until the desired delay has past
int16_t repeat(uint16_t* instructionBuffer, int16_t delay, uint32_t asm_instr, uint8_t *length, uint16_t max_delay) {
    while(delay > 0){
        uint8_t delay_part = min(max_delay, delay) - 1;
        instructionBuffer[(*length)] = asm_instr | (((max_delay-1) & delay_part) << 8);
        delay = delay - (delay_part + 1);
        (*length)++;
    }
}

// how many instructions are needed to create this delay?
uint8_t instructionCount(uint16_t delay, uint16_t max_delay){
    if (delay % max_delay == 0){
        return delay/max_delay;
    } else {
        return delay/max_delay + 1;
    }
}

/*** helper functions to implement baseband phase shifting ***/


/* returns: information about the sequence of values in a phase-shifted square wave,
            as well as the number of cycles to spend on each value
   parameter: square_wave <- a pointer to a square_wave_t struct, see definition in header
   parameter: phase_shift <- phase shift in degrees (e.g. 90, 180, 270...) (precondition: must be positive)
   parameter: cycles_per_period <- cycles per square wave period (e.g. d0 or d1) (precondition: must be even)
   status: tested and seems to work well.
   NOTE: currently not in use due to PIO memory limitations, we use an initial baseband delay instead.
*/
uint8_t phase_shift_square_wave(square_wave_t *square_wave, uint16_t phase_shift, uint16_t cycles_per_period) {

  uint16_t cycles_per_sign = cycles_per_period/2; // no remainder due to precondition
  phase_shift = phase_shift % 360; // wrap around phase

  square_wave->values[0] = (phase_shift < 180) ? 1 : 0; // starting value depends on phase

  square_wave->cycles[0] = cycles_per_sign - (uint16_t) roundf(
                             (float)cycles_per_sign*((float)(phase_shift % 180)/180.0f)
                           );

  square_wave->cycles[0] = (square_wave->cycles[0] == 0) ? 1 : square_wave->cycles[0]; // ensure a nonzero value
  square_wave->cycles[0] = (square_wave->cycles[0] == cycles_per_sign) ? square_wave->cycles[0]-1 : square_wave->cycles[0]; // ensure a nonzero value

  square_wave->values[1] = (phase_shift < 180) ? 0 : 1; // first sign change always happens

  square_wave->cycles[1] = (phase_shift == 0 || phase_shift == 180) ? // ~ quarter period (half-period if repeated)
                            cycles_per_sign/2 : cycles_per_sign;

  square_wave->values[2] = (phase_shift == 0 || phase_shift == 180) ? // potentially a second sign change
                            square_wave->values[1] : square_wave->values[0];

  square_wave->cycles[2] = (phase_shift == 0 || phase_shift == 180) ? // remaining cycles to fill the period
                            cycles_per_sign - square_wave->cycles[1] : cycles_per_sign - square_wave->cycles[0];
}


/*
   returns: the number of CPU cycles (and PIO cycles, assuming no clock divider is used)
            corresponding to the time delay required to emulate a certain phase shift at
            the baseband center frequency (f0 + f1)/2.
   parameter: phase_shift <- the desired phase shift in degrees (positive, i.e. 0, 15, 90, 360, 720...)
   parameter: d0 <- cycles per period in square wave for symbol 0
   parameter: d1 <- cycles per period in square wave for symbol 1
   status: tested, seems to work
   NOTE: since delay has to be an integer number of cycles, the number and magnitude of realizable
         phase shifts is limited and depends on d.
         more fine-grained phase control can be achieved by increasing d (reducing the baseband center frequency)
         (IDEA: the frequency reduction can then be compensated by overclocking the RP2040 instead)
*/
uint16_t phase_shift_to_delay_cycles(uint16_t phase_shift, uint16_t d0, uint16_t d1) {

  phase_shift = phase_shift % 360; // wrap around
  uint16_t d = (d0 + d1)/2; // period of baseband center frequency in terms of cycles

  uint16_t delay_cycles = (uint16_t) roundf( (float)phase_shift/360.0f * (float)d);
  return delay_cycles;
}


/********************************************/


/*
changes from main branch:
  - delay baseband signal by appropriate number of cycles (pulled from FIFO) to emulate desired phase shift at baseband center freq.
  - removed dual antennae mode in favor of using a single SM per antenna
*/
bool generatePIOprogram(uint16_t d0,uint16_t d1, uint32_t baud, uint16_t* instructionBuffer, struct pio_program *backscatter_program) {

    // compute label positions
    uint16_t MAX_ASMDELAY = 0x0020; // 32
    uint16_t OPT_SIDE_1   = 0x0000;
    uint16_t OPT_SIDE_0   = 0x0000;

    uint8_t phase_delay_label = 4;
    uint8_t get_symbol_label = 5;
    uint8_t send_1_label = 7;
    uint8_t loop_1_label = send_1_label + 1;
    int16_t lastPeriodCycles1 = (((uint32_t) CLKFREQ*1000000)/baud - 4) % ((uint32_t) d1);
    int16_t lastPeriodCycles0 = (((uint32_t) CLKFREQ*1000000)/baud - 4) % ((uint32_t) d0);
    int16_t tmp1 = min(lastPeriodCycles1, d1/2);
    int16_t tmp0 = min(lastPeriodCycles0, d0/2);
    /*                                           pull high                 pull low            jmp                 high                                      low                            jmp  */
    uint8_t send_0_label = loop_1_label + instructionCount(d1/2, MAX_ASMDELAY) + instructionCount(d1/2 - 1, MAX_ASMDELAY) + 1 + instructionCount(tmp1, MAX_ASMDELAY) + instructionCount(max(0,lastPeriodCycles1-tmp1), MAX_ASMDELAY) + 1;
    uint8_t loop_0_label = send_0_label + 1;

    // check that the program will fit into memory
    /*                      pull high                pull low            jmp                        high                              low                               jmp  */
    if(loop_0_label + instructionCount(d0/2, MAX_ASMDELAY) + instructionCount(d0/2 - 1, MAX_ASMDELAY) + 1 + instructionCount(tmp0, MAX_ASMDELAY) + instructionCount(max(0,lastPeriodCycles0-tmp0), MAX_ASMDELAY) + 1 >= 32){
        printf("ERROR: The clock dividers are too small. The program would not fit into the state-machine instruction memory. Alternatively, you can disable the second antenna. This increaes the maximal delay per instruction from 8 to 32 cycles and thus significanlty reduces the required code space.");
        return false;
    }

    // generate state machine
    instructionBuffer[0] = ASM_SET_PINS | OPT_SIDE_1 | 1;            //  0: set    pins, 1         side 1

    // pull symbol times (d0 and d1) from FIFO into registers ISR and Y
    instructionBuffer[1] = ASM_OUT | (ASM_ISR_REG << 5);             //  1: out    isr, 32   (NOTE: 32=0)
    instructionBuffer[2] = ASM_OUT | (ASM_Y_REG   << 5);             //  2: out    y, 32     (NOTE: 32=0)

    // pull phase delay (in cycles) from FIFO into register X and stall for that number of cycles
    instructionBuffer[3] = ASM_OUT | (ASM_X_REG << 5);               //  3: out    x, 32     (NOTE: 32=0)
    instructionBuffer[4] = ASM_JMP_XMM | (0x1F & phase_delay_label); //  .phase_delay_label 4: jmp    x--, phase_delay_label

    instructionBuffer[5] = ASM_OUT | (ASM_X_REG   << 5) |  1;        //  .get_symbol_label  5: out    x, 1
    instructionBuffer[6] = ASM_JMP_NOTX | (0x1F & send_0_label);     //  6: jmp    !x, send_0_label
    /*       symbol 1      */
    instructionBuffer[7] = ASM_MOV | (ASM_X_REG << 5) | ASM_Y_REG;   //  .send_1_label  7: mov    x, y

    uint8_t length = 8;
    // full periods
    repeat(instructionBuffer, d1/2,     ASM_SET_PINS | OPT_SIDE_1 | 1, &length, MAX_ASMDELAY);   //  .loop_1_label  8: set    pins, 1         side 1 [delay] 
    repeat(instructionBuffer, d1/2 - 1, ASM_SET_PINS | OPT_SIDE_0 | 0, &length, MAX_ASMDELAY);   //  ...: set    pins, 0         side 0 [delay] 
    instructionBuffer[length] = ASM_JMP_XMM | (0x1F & loop_1_label);                             //  ...: jmp    x--, loop_1_label
    length++;

    // remaining period to fill symbol time
    repeat(instructionBuffer,                          tmp1, ASM_SET_PINS | OPT_SIDE_1 | 1, &length, MAX_ASMDELAY); //  ...: set    pins, 1         side 1 [delay] 
    repeat(instructionBuffer, max(0,lastPeriodCycles1-tmp1), ASM_SET_PINS | OPT_SIDE_0 | 0, &length, MAX_ASMDELAY); //  ...: set    pins, 0         side 0 [delay] 
    instructionBuffer[length] = ASM_JMP | get_symbol_label;               // ...: jmp    get_symbol_label
    length++;

    /*       symbol 0       */
    instructionBuffer[length] = ASM_MOV | (ASM_X_REG << 5) | ASM_ISR_REG, // .send_0_label  ...: mov    x, isr
    length++;

    // full periods
    repeat(instructionBuffer, d0/2,     ASM_SET_PINS | OPT_SIDE_1 | 1, &length, MAX_ASMDELAY);    // .loop_0_label  ...: set    pins, 1         side 1 [delay_part] 
    repeat(instructionBuffer, d0/2 - 1, ASM_SET_PINS | OPT_SIDE_0 | 0, &length, MAX_ASMDELAY);    // ...: set    pins, 0         side 0 [delay_part] 
    instructionBuffer[length] = ASM_JMP_XMM | (0x1F & loop_0_label);      //  ...: jmp    x--, loop_0_label
    length++;

    // remaining period to fill symbol time
    repeat(instructionBuffer,                          tmp0, ASM_SET_PINS | OPT_SIDE_1 | 1, &length, MAX_ASMDELAY);  //  ...: set    pins, 1         side 1 [delay_part] 
    repeat(instructionBuffer, max(0,lastPeriodCycles0-tmp0), ASM_SET_PINS | OPT_SIDE_0 | 0, &length, MAX_ASMDELAY);  //  ...: set    pins, 0         side 0 [delay_part] 
    instructionBuffer[length] = ASM_JMP | get_symbol_label; // ...: jmp    get_symbol_label

    // configure program origin and length
    backscatter_program->instructions = instructionBuffer;
    backscatter_program->length = length+1;
    backscatter_program->origin = -1;
    return true;
}


/*
    - based on d0/d1/baud, the modulation parameters will be computed and returned in the struct backscatter_config

    changes from main branch:
    - changed to accomodate a configurable number of antennae (1 to 4), each controlled by a separate sm in the same PIO block
    - the state machines are initialized but not started, and are instead (re)started in sync when a message is sent
    - phase delays are not set at initialization, but are instead set individually for each message

    parameter: sm <- array of claimed state machine id's (maximum of 4 due to residing inside a single PIO block)
    parameter: pins <- array of pin id's, mapping to each state machine
    parameter: num_antennae <- number of antennae to use (1 to 4)
*/
void backscatter_program_init(PIO pio, uint8_t *state_machines, uint16_t *pins, uint8_t num_antennae, uint16_t d0, uint16_t d1, uint32_t baud, struct backscatter_config *bs_config, uint16_t *instructionBuffer) {

    pio_set_sm_mask_enabled(pio, 0xF, false); // stop state machines (if running)

    // print warning at invalid settings
    if(d0 % 2 != 0){
        printf("WARNING: the clock divider d0 has to be an even integer. The state-machine may not function correctly");
    }
    if(d1 % 2 != 0){
        printf("WARNING: the clock divider d1 has to be an even integer. The state-machine may not function correctly");
    }

    // correct baud-rate
    if(((uint32_t) (CLKFREQ*pow(10,6))) % baud != 0){
        uint32_t baud_new = round(((uint32_t) (CLKFREQ*pow(10,6))) / round(((double) CLKFREQ*pow(10,6)) / ((double) baud)));
        printf("WARNING: a baudrate of %d Baud is not achievable with a %d MHz clock.\nTherefore, the closest achievable baud-rate %d Baud will be used.\n", baud, CLKFREQ, baud_new);
        baud = baud_new;
    }

    // generate pio-program
    struct pio_program backscatter_program;
    generatePIOprogram(d0, d1, baud, instructionBuffer, &backscatter_program);
    uint offset = 0;
    pio_add_program_at_offset(pio, &backscatter_program, offset); // load program

    /* print state-machine instructions */
    //printf("state-machine length: %d\n", backscatter_program.length);
    //for (uint16_t t = 0; t < backscatter_program.length; t++){
    //    printf("0x%04x\n",backscatter_program.instructions[t]);
    //}

    // configure one state machine for each antenna
    for(int i = 0; i < num_antennae; i++) {
       pio_gpio_init(pio, pins[i]);
       pio_sm_set_consecutive_pindirs(pio, state_machines[i], pins[i], 1, true);
    }

    // compute number of periods per symbol
    uint32_t reps0 = ((CLKFREQ*1000000/baud - 4) / d0) - 1; // -1 is required since JMP 0-- is still true
    uint32_t reps1 = ((CLKFREQ*1000000/baud - 4) / d1) - 1; // -1 is required since JMP 0-- is still true

    pio_sm_config config[num_antennae];
    for(int i = 0; i < num_antennae; i++) {
      // setup default state-machine config
      config[i] = pio_get_default_sm_config();
      sm_config_set_wrap(&config[i], offset, offset + backscatter_program.length-1);
      // setup specific state-machine config
      sm_config_set_set_pins(&config[i], pins[i], 1);
      sm_config_set_fifo_join(&config[i], PIO_FIFO_JOIN_TX); // We only need TX, so get an 8-deep FIFO (join RX and TX FIFO)
      sm_config_set_out_shift(&config[i], false, true, 32);  // OUT shifts to left (MSB first), autopull after every 32 bit
      pio_sm_init(pio, state_machines[i], offset, &config[i]);

      // feed TX FIFOs with symbol durations
      pio_sm_put_blocking(pio, state_machines[i], reps0);
      pio_sm_put_blocking(pio, state_machines[i], reps1);
    }

    // compute configuration parameters
    uint32_t fcenter    = (CLKFREQ*1000000/d0 + CLKFREQ*1000000/d1)/2;
    uint32_t fdeviation = abs(round((((double) CLKFREQ*1000000)/((double) d1)) - ((double) fcenter)));
    bs_config->baudrate    = baud;
    bs_config->center_offset = round(fcenter);
    bs_config->deviation   = round(fdeviation);
    bs_config->minRxBw     = round((baud + 2*fdeviation));

    if (fdeviation > 380000){
        printf("WARNING: the deviation is too large for the CC2500\n");
    }
    if (fdeviation > 1000000){
        printf("WARNING: the deviation is too large for the CC1352\n");
    }
    if (d0 < d1){
        printf("WARNING: symbol 0 has been assigned to larger frequncy than symbol 1\n");
    }
    printf("Computed baseband settings: \n- baudrate: %d\n- Center offset: %d\n- deviation: %d\n- RX Bandwidth: %d\n", bs_config->baudrate, bs_config->center_offset, bs_config->deviation, bs_config->minRxBw);
}


// TODO: setup DMA channels to feed Tx FIFOs (if necessary after testing, might not be needed though!)
/*
    changes from main branch:
    - changed to accomodate a configurable number of antennae (1 to 4), each controlled by a separate sm in the same PIO block    
    - the state machines are (re)started in sync (at the phase delay loop) each time a new message is sent

    parameter: sm <- array of claimed state machine id's (maximum of 4 due to residing inside a single PIO block)
    parameter: phase_delay_cycles <- array of phase delays, in cycles per antennae (same antennae order as pin order)
    parameter: num_antennae <- number of antennae to use (1 to 4)
*/
void backscatter_send(PIO pio, uint8_t *state_machines, uint16_t *phase_delay_cycles, uint8_t num_antennae, uint32_t *message, uint32_t len) {

    /*
       - when the PIO program runs the first time it starts at the first instruction
         and pulls symbol durations from the TX FIFOs
       - when the PIO program is restarted to send a new message it should instead start at
         instruction #3, pulling the new phase delays from the TX FIFOs
       - to differentiate the first invocation from subsequent ones we branch on a static variable
    */
    static bool first_call = true;

    if(!first_call) {
      /* at subsequent calls we also need to:
          - stop all running state machines
          - set their PCs to fetch the new phase delay at startup (i.e. restart at instruction #3)
      */
      pio_set_sm_mask_enabled(pio, 0xF, false);
      for(int i = 0; i < num_antennae; i++) {
        pio_sm_exec(pio, state_machines[i], ASM_JMP | 3);
      }
      first_call = false;
    }

    for(int i = 0; i < num_antennae; i++) {
      // feed TX FIFOs with phase delays
      pio_sm_put_blocking(pio, state_machines[i], phase_delay_cycles[i]);

      // put the first word of the message into all TX FIFOs, so that transmissions can begin immediately in sync
      pio_sm_put_blocking(pio, state_machines[i], message[0]);
    }

    pio_enable_sm_mask_in_sync(pio, 0xF); // start all state machines in sync

    for(uint32_t i = 1; i < len; i++){
      for(int j = 0; j < num_antennae; j++) {
        pio_sm_put_blocking(pio, state_machines[j], message[i]); // refill all TX FIFOs using the CPU (TODO: use DMA instead)
      }
    }
    sleep_ms(1); // wait for transmission to finish
}
