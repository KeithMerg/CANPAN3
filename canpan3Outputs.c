/*
  This work is licensed under the:
      Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
   To view a copy of this license, visit:
      http://creativecommons.org/licenses/by-nc-sa/4.0/
   or send a letter to Creative Commons, PO Box 1866, Mountain View, CA 94042, USA.

   License summary:
    You are free to:
      Share, copy and redistribute the material in any medium or format
      Adapt, remix, transform, and build upon the material

    The licensor cannot revoke these freedoms as long as you follow the license terms.

    Attribution : You must give appropriate credit, provide a link to the license,
                   and indicate if changes were made. You may do so in any reasonable manner,
                   but not in any way that suggests the licensor endorses you or your use.

    NonCommercial : You may not use the material for commercial purposes. **(see note below)

    ShareAlike : If you remix, transform, or build upon the material, you must distribute
                  your contributions under the same license as the original.

    No additional restrictions : You may not apply legal terms or technological measures that
                                  legally restrict others from doing anything the license permits.

   ** For commercial use, please contact the original copyright holder(s) to agree licensing terms

    This software is distributed in the hope that it will be useful, but WITHOUT ANY
    WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE
 */
/**
 *	The CANPAN program.
 * This handles driving the LED outputs.
 * TMR0 is already used by Ticktime, here TMR2 is used to strobe through the 
 * row/columns of the LED matrix and TMR1 is used for the LED brightness control.
 *
 * @author Ian Hogg 
 * @date October 2024
 * 
 */ 
#include <xc.h>
#include "module.h"
#include "canpan3Outputs.h"
#ifndef _XTAL_FREQ   // KeithB b27
#define _XTAL_FREQ  (clkMHz*1000000UL)   // for __delay_us()
#endif
#include "canpan3Nv.h"
#include "nv.h"

/* METHOD 1 uses software to loop through all the anodes, sending a byte of
 * cathode data to SPI. Software also used to turn off the cathode when brightness
 * value reached.
 * METHOD 2 similar to METHOD 1 but uses DMA to send data to SPI to turn off
 * LEDs at the correct time due to brightness control.
 * METHOD 3 Turns on each cathode in turn via SPI and uses PWM peripherals to
 * drive the anodes.  
 */

// LED_ROW_[1-4] are used to drive the LED Anodes
// MSSP SSI Master is used to provide 8 bits for cathodes

#define MAX_BRIGHTNESS  32 // Must be power of two.

static uint8_t brightness = 0;
static uint8_t current_row = 0;
static uint8_t ledMatrix[8];   // KeithB b14-25: 8 rows for CANDISP (4 used on CANPAN3)
// KeithB b40: brightness NVs cached in RAM (kept current by updateLedBrightness() from
// APP_nvValueChanged) so the ISR never calls getNV(). Before this every row change made
// 8 (16 on CANDISP) getNV() calls inside the interrupt, and because getNV() was reachable
// from both mainline and the ISR the compiler carried a second copy of it.
static uint8_t ledBright[NUM_LEDS ? NUM_LEDS : 1];
// KeithB b33: row brightness pointer set at each row change (b40: points into ledBright)
static uint8_t *rowBright0;   // KeithB b42: RAM pointer, not const (keeps XC8 from generating a mixed-space accessor)
#if HARDWARE==HW_CANDISP   // KeithB b14-25
static uint8_t *rowBright1;
#endif
// KeithB b40: bit mask table; a variable shift on the PIC18 is a loop
static const uint8_t bitMask[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

/**
 * Refresh the cached brightness for one LED. Called from APP_nvValueChanged() and at start-up.
 */
// KeithB b40
void updateLedBrightness(uint8_t ledNo, uint8_t value) {
    if (ledNo < NUM_LEDS) {
        ledBright[ledNo] = value;
    }
}

void initOutputs(void) {
    uint8_t i;   // KeithB b40
    // KeithB b14-25: pins via module.h macros
    for (i=0; i<NUM_LEDS; i++) {   // KeithB b40: load the brightness cache from the NVs
        ledBright[i] = (uint8_t)getNV(NV_BRIGHTNESS + i);
    }
    rowBright0 = ledBright;
#if HARDWARE==HW_CANDISP
    rowBright1 = ledBright + 8;
#endif
    ledMatrix[3] = ledMatrix[2] = ledMatrix[1] = ledMatrix[0] = 0;
#if HARDWARE==HW_CANDISP   // KeithB b14-25
    ledMatrix[7] = ledMatrix[6] = ledMatrix[5] = ledMatrix[4] = 0;
#endif
    TRIS_LED_ROW_1 = 0;   // anode driver output
    TRIS_LED_ROW_2 = 0;   // anode driver output
    TRIS_LED_ROW_3 = 0;   // anode driver output
    TRIS_LED_ROW_4 = 0;   // anode driver output
    
    LAT_LED_ROW_1 = 0;    // LED anode drivers off
    LAT_LED_ROW_2 = 0;
    LAT_LED_ROW_3 = 0;
    LAT_LED_ROW_4 = 0;
    
    // Cathode driver output enable
    TRIS_TLC5917_13 = 0;
    LAT_TLC5917_13 = 0;     // disabled.
    
    // latch
    TRIS_TLC5917__4 = 0;
    LAT_TLC5917__4 = 0;     // unlatch LE
    
    //Set up the MSSP to drive the LED matrix
    TRIS_TLC5917__3 = 0;   //clock
    LAT_TLC5917__3 = 0;
    TRIS_TLC5917__2 = 0;   // data
    LAT_TLC5917__2 = 0;
    
    SPI1CON0 = 0x03; // MSb first, host mode, Total bit count Mode=1, transmit only
    SPI1CON1 = 0x44; // clock edge, SSP=0 for active low and latches when it goes low
    SPI1CON2 = 0x02; // transmitter on, receiver off
        
    SPI1TCNTH=0;     // 1 byte
    SPI1TCNTL=1;     // 1 byte
    SPI1TWIDTH=0;    // 8 bits

    SPI1CLK = 0x00; // Clock from Fosc
    SPI1BAUD = 7;  // Fosc/16

    // set up PPS to the correct pins
    RC5PPS = 0x32; // SPI1SDO
    RC3PPS = 0x31; // SPI1SCK
    RC4PPS = 0x33; // SPI1SS, latch

    // ready
    SPI1CON0bits.EN = 1;

#ifdef LED_MATRIX_ISR   // KeithB b26
    // TMR2 drives pollOutputs() at a fixed rate so the software PWM step
    // time does not depend on main loop load.
    // Clock Fosc/4 = clkMHz/4 MHz, prescaler 1:16 -> (clkMHz/64) MHz.
    // Period = LED_MATRIX_ISR_PERIOD_US * clkMHz/64 - 1 (64MHz: 100us -> 99)
    _Static_assert((LED_MATRIX_ISR_PERIOD_US * clkMHz / 64) - 1 <= 255, "LED_MATRIX_ISR_PERIOD_US too large for TMR2");
    T2CONbits.ON = 0;
    T2CLKCON = 0x01;        // clock source Fosc/4
    T2HLT = 0x00;           // free running, software gate
    T2PR = (uint8_t)((LED_MATRIX_ISR_PERIOD_US * clkMHz / 64) - 1);
    T2TMR = 0;
    T2CON = 0x40;           // prescaler 1:16, postscaler 1:1
    TMR2IP = 0;             // low priority (below the ticktime TMR0 interrupt)
    TMR2IF = 0;
    TMR2IE = 1;
    T2CONbits.ON = 1;
#endif
}

#ifdef LED_MATRIX_ISR   // KeithB b26
/**
 * TMR2 interrupt: one LED matrix brightness step per period.
 */
void __interrupt(irq(TMR2), base(IVT_BASE), low_priority) TMR2_ISR(void) {   // KeithB b49: matches TMR2IP = 0 now the library enables priorities
    TMR2IF = 0;
    pollOutputs();
}
#endif

/**
 * Shift the cathode data out to the TLC5917(s) and pulse LE so it appears
 * on the outputs. Second byte only used by CANDISP.
 */
// KeithB b28: SPI transfer + LE in one place; b33: 2-byte count on CANDISP
static void latchCathodes(uint8_t c0, uint8_t c1) {
    SPI1TCNTH=0;
#if HARDWARE==HW_CANDISP   // KeithB b14-25
    SPI1TCNTL=2;     // 2 bytes: SS (LE) spans both cascaded TLC5917s
#else
    SPI1TCNTL=1;     // 1 byte
#endif
    SPI1TWIDTH=0;    // 8 bits
    SPI1TXB = c0;
#if HARDWARE==HW_CANDISP   // KeithB b14-25
    SPI1TXB = c1;
#else
    (void)c1;
#endif
    // Ensure all bits have physically shifted out before hitting LE.
#ifdef LED_SPI_WAIT_TXBE   // KeithB b30
    while (! SPI1STATUSbits.TXBE)
        ;
#else
    while (SPI1CON2bits.BUSY)
        ;
#endif
    LAT_TLC5917__4 = 1; // LE
    // TLC5917: LE pulse width > 20ns, LE to OUT up to 365ns (5 cycles at 64MHz)
    NOP();
    NOP();
    LAT_TLC5917__4 = 0; // LE
    NOP();
    NOP();
    NOP();
    NOP();
    NOP();
}

void pollOutputs(void)
{
    uint8_t i;
    static uint8_t cathodes0;
    static uint8_t cathodes1;
//    static uint8_t rowBrightness[4] = {16,1,16,6};
    
    if (brightness == 0) {
        // move to next row (0 - 3)
#ifdef LED_GHOST_SUPPRESS   // KeithB b28
        uint8_t prev_row = current_row;
        uint8_t dis0;
#if HARDWARE==HW_CANDISP   // KeithB b14-25
        uint8_t dis1;
#endif
#endif
        current_row++;

        _Static_assert((NUM_LED_ROWS & (NUM_LED_ROWS-1)) == 0, "NUM_LED_ROWS must be a power of 2");
        current_row &= NUM_LED_ROWS-1;

        // KeithB b14-25: two cathode bytes for CANDISP
        cathodes0 = ledMatrix[current_row];
#if HARDWARE==HW_CANDISP   // KeithB b14-25
        cathodes1 = ledMatrix[4 + current_row];
#else
        cathodes1 = 0;
#endif
        // KeithB b40: point at this row's cached brightness values (no getNV() in the ISR)
        rowBright0 = ledBright + current_row * NUM_LED_COLUMNS;
#if HARDWARE==HW_CANDISP   // KeithB b14-25
        rowBright1 = rowBright0 + 8;
#endif

#ifdef LED_GHOST_SUPPRESS   // KeithB b28
        // Step 1: sinks off while the previous anode is still on. Columns
        // that were being sunk charge up through the previous row's lit
        // LEDs (harmless - they were lit). Columns that were not sunk are
        // already high.
        LAT_TLC5917_13 = 1; // OE off
        __delay_us(LED_GHOST_PRECHARGE_US);

        // Step 2: discharge the previous row line through LEDs that are lit
        // in both the previous and the next row. Those columns get pulled
        // low again but they are about to be sunk in the next row anyway.
        // If there is no such LED fall back to the previous row's own lit
        // LEDs (still invisible, but leaves those columns low).
        dis0 = ledMatrix[prev_row] & cathodes0;
#if HARDWARE==HW_CANDISP   // KeithB b14-25
        dis1 = ledMatrix[4 + prev_row] & cathodes1;
        if ((dis0 | dis1) == 0) {
            dis0 = ledMatrix[prev_row];
            dis1 = ledMatrix[4 + prev_row];
        }
#else
        if (dis0 == 0) {
            dis0 = ledMatrix[prev_row];
        }
#endif
        if (dis0
#if HARDWARE==HW_CANDISP   // KeithB b14-25
                | dis1
#endif
                ) {
            latchCathodes(dis0,
#if HARDWARE==HW_CANDISP   // KeithB b14-25
                          dis1
#else
                          0
#endif
                          );
            // anodes off, then sinks on: the previous row line discharges
            LAT_LED_ROW_1 = 0;
            LAT_LED_ROW_2 = 0;
            LAT_LED_ROW_3 = 0;
            LAT_LED_ROW_4 = 0;
            LAT_TLC5917_13 = 0; // OE on
            __delay_us(LED_GHOST_DISCHARGE_US);
            LAT_TLC5917_13 = 1; // OE off
        } else {
            LAT_LED_ROW_1 = 0;
            LAT_LED_ROW_2 = 0;
            LAT_LED_ROW_3 = 0;
            LAT_LED_ROW_4 = 0;
        }
#else
        // disable the cathode driver
        LAT_TLC5917_13 = 1; // OE

        // also turn the anodes off
        LAT_LED_ROW_1 = 0;
        LAT_LED_ROW_2 = 0;
        LAT_LED_ROW_3 = 0;
        LAT_LED_ROW_4 = 0;
#endif

        // Step 3: latch the next row's data.
        // For CANDISP, 1st TLC5917 e.g. LED 1- 8, 17-24, 33-40, 49-56.
        // CANPAN3 has only one TLC5917, so cathode[1] isn't needed.
        // 2nd TLC5917 e.g. LED 9-16, 25-32, 41-48, 57-64.
        latchCathodes(cathodes0, cathodes1);   // KeithB b26: duplicate brightness += 2 removed from this step

#ifdef LED_ROW_BLANK_US   // KeithB b27
        // Dead time with OE disabled and all anodes off (only useful if the
        // row/column lines have bleed resistors fitted).
        __delay_us(LED_ROW_BLANK_US);
#endif

        // KeithB b28: empty rows never energised (only with LED_GHOST_SUPPRESS)
        // Turn the next row's anode on.
#ifdef LED_GHOST_SUPPRESS   // KeithB b28
        // A row with nothing lit is never energised so its line never
        // charges and can never ghost.
        if (cathodes0 | cathodes1)
#endif
        switch (current_row) {
            case 0:
                LAT_LED_ROW_1 = 1;
                break;
            case 1:
                LAT_LED_ROW_2 = 1;
                break;
            case 2:
                LAT_LED_ROW_3 = 1;
                break;
            case 3:
                LAT_LED_ROW_4 = 1;
                break;
        }

        // enable the cathode driver
        LAT_TLC5917_13 = 0; //OE
    } else {
#ifdef LED_DIAG_NO_DIMMING   // KeithB b31
        // Diagnostic: leave the row exactly as written at the row change.
#else
        // Same row but turn off any LEDs that have their brightness setting
        // less than the current brightness. We do NOT change the anodes here.
        // KeithB b33: compute first; blank and transfer only when changed.
        uint8_t new0 = cathodes0;
        uint8_t new1 = cathodes1;
#ifdef LED_FAST_DIM
        // KeithB b40: rolling mask and walking pointer; the variable shift (1U << i) compiled
        // to a rotate loop and the array index was rebuilt every iteration - ~20us of the
        // 100us ISR period. Comment out LED_FAST_DIM in module.h to compare with the old loop.
        {
            uint8_t mask = 1;
            uint8_t *rb0 = rowBright0;
#if HARDWARE==HW_CANDISP
            uint8_t *rb1 = rowBright1;
#endif
            for (i=0; i<8; i++) {
                if (brightness > *rb0++) {
                    new0 &= (uint8_t)~mask;
                }
#if HARDWARE==HW_CANDISP
                if (brightness > *rb1++) {
                    new1 &= (uint8_t)~mask;
                }
#endif
                mask <<= 1;
            }
        }
#else
        for (i=0; i<8; i++) {
            if (brightness > rowBright0[i]) {
                new0 &= (uint8_t)~(1U << i);
            }
#if HARDWARE==HW_CANDISP   // KeithB b14-25
            if (brightness > rowBright1[i]) {
                new1 &= (uint8_t)~(1U << i);
            }
#endif
        }
#endif
        if ((new0 != cathodes0) || (new1 != cathodes1)) {
            cathodes0 = new0;
            cathodes1 = new1;
            // Disable the cathode driver while the data ripples through the
            // transparent TLC5917 latch, then re-enable it.
            LAT_TLC5917_13 = 1; // OE off
            latchCathodes(cathodes0, cathodes1);
            LAT_TLC5917_13 = 0; // OE on
        }
#endif
    }
    brightness += 2;
    
    _Static_assert((MAX_BRIGHTNESS & (MAX_BRIGHTNESS-1)) == 0, "MAX_BRIGHTNESS must be a power of 2");
    brightness &= MAX_BRIGHTNESS-1;     // Wrap at MAX_BRIGHTNESS-1
}


/**
 * Turn on an LED. No is 0-31/63.
 * @param no
 */
// KeithB b14-25: CANDISP LED numbering
void setLed(uint8_t no) {
#if HARDWARE==HW_CANDISP   // KeithB b14-25
    ledMatrix[((no&0x08) == 0x08)*4 + no/NUM_LED_COLUMNS] |= bitMask[no & 7];   // KeithB b40: table, not a shift loop
#else
    ledMatrix[no/NUM_LED_COLUMNS] |= bitMask[no & 7];   // KeithB b40: table, not a shift loop
#endif
}

/**
 * Turn an LED off. No is 0-31/63
 */
// KeithB b14-25: CANDISP LED numbering
void clearLed(uint8_t no) {
#if HARDWARE==HW_CANDISP   // KeithB b14-25
    ledMatrix[((no&0x08) == 0x08)*4 + no/NUM_LED_COLUMNS] &= (uint8_t)~bitMask[no & 7];   // KeithB b40
#else
    ledMatrix[no/NUM_LED_COLUMNS] &= (uint8_t)~bitMask[no & 7];   // KeithB b40
#endif
}

/**
 * Test if an LED is on. No is 0-31/63
 * @param no
 * @return 0 if OFF or non zero if ON
 */
// KeithB b14-25: CANDISP LED numbering
uint8_t testLed(uint8_t no) {
#if HARDWARE==HW_CANDISP   // KeithB b14-25
    return ledMatrix[((no&0x08) == 0x08)*4 + no/NUM_LED_COLUMNS] & bitMask[no & 7];   // KeithB b40
#else
    return ledMatrix[no/NUM_LED_COLUMNS] & bitMask[no & 7];   // KeithB b40
#endif
}

