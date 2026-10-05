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

// LAT_LED_ROW_1..4 drive the LED anodes
// MSSP SSI Master is used to provide 8 bits for cathodes

unsigned char ledMatrix[NUM_LED_ROWS];
static unsigned char current_row = 0;
static uint8_t cathodes;
#define MAX_BRIGHTNESS  32  // must be a power of 2
static uint8_t brightness = 0;
// The brightness NVs, cached so that pollOutputs() (an interrupt) never calls
// getNV(). Kept up to date by updateLedBrightness() from APP_nvValueChanged().
static uint8_t ledBright[NUM_LEDS];
static uint8_t *rowBright;      // this row's entries in ledBright[]
// bit masks: a variable shift (1 << n) is a loop on the PIC18
static const uint8_t bitMask[8] = {0x01,0x02,0x04,0x08,0x10,0x20,0x40,0x80};

/**
 * Update the cached brightness of one LED.
 * @param ledNo the LED 0-31
 * @param value the brightness NV value
 */
void updateLedBrightness(uint8_t ledNo, uint8_t value) {
    if (ledNo < NUM_LEDS) {
        ledBright[ledNo] = value;
    }
}

void initOutputs(void) {
    uint8_t i;
    
    for (i=0; i< NUM_LED_ROWS; i++) {
        ledMatrix[i] = 0;
    }
    for (i=0; i<NUM_LEDS; i++) {
        ledBright[i] = (uint8_t)getNV(NV_BRIGHTNESS + i);
    }
    rowBright = ledBright;
    TRIS_LED_ROW_3 = 0;   // anode driver output
    TRIS_LED_ROW_4 = 0;   // anode driver output
    TRIS_LED_ROW_1 = 0;   // anode driver output
    TRIS_LED_ROW_2 = 0;   // anode driver output
    
    LAT_LED_ROW_1 = 0;    // LED anode drivers off
    LAT_LED_ROW_2 = 0;
    LAT_LED_ROW_3 = 0;
    LAT_LED_ROW_4 = 0;
    
    // Cathode driver output enable
    TRIS_TLC5917_13 = 0;
    LAT_TLC5917_13 = 0;     // disabled.
    
    // latch
    TRIS_TLC5917__4 = 0;
    LAT_TLC5917__4 = 0;     // unlatch
    
    //Set up the MSSP to drive the switch matrix
    TRIS_TLC5917__3 = 0;   //clock
    LAT_TLC5917__3 = 0;
    TRIS_TLC5917__2 = 0;   // data
    LAT_TLC5917__2 = 0;
    
    SPI1CON0 = 0x03; // MSb first, host mode, Total bit count Mode=1, transmit only
    SPI1CON1 = 0x44; // clock edge, SSP=0 for active low and latches when it goes low
    SPI1CON2 = 0x02; // transmitter on, receiver off
        
    SPI1TCNTH=0;     // 1 byte
    SPI1TCNTL=1;     // 1 byte
    SPI1TWIDTH=0;   // 8 bits

    SPI1CLK = 0x00; // Clock from Fosc
    SPI1BAUD = 7;  // Fosc/16

    // set up PPS to the correct pins
    RC5PPS = 0x32; // SPI1SDO
    RC3PPS = 0x31; // SPI1SCK
    RC4PPS = 0x33; // SPI1SS, latch

    // ready
    SPI1CON0bits.EN = 1;

#ifdef LED_MATRIX_ISR
    // TMR2 calls pollOutputs() at a fixed rate.
    // Clock Fosc/4 = clkMHz/4 MHz, prescaler 1:16 -> clkMHz/64 MHz.
    // Period = LED_MATRIX_ISR_PERIOD_US * clkMHz/64 - 1 (64MHz: 100us -> 99)
    _Static_assert((LED_MATRIX_ISR_PERIOD_US * clkMHz / 64) - 1 <= 255, "LED_MATRIX_ISR_PERIOD_US too large for TMR2");
    T2CONbits.ON = 0;
    T2CLKCON = 0x01;        // clock source Fosc/4
    T2HLT = 0x00;           // free running, software gate
    T2PR = (uint8_t)((LED_MATRIX_ISR_PERIOD_US * clkMHz / 64) - 1);
    T2TMR = 0;
    T2CON = 0x40;           // prescaler 1:16, postscaler 1:1
    TMR2IP = 0;             // low priority
    TMR2IF = 0;
    TMR2IE = 1;
    T2CONbits.ON = 1;
#endif
}

#ifdef LED_MATRIX_ISR
/**
 * TMR2 interrupt: one LED matrix brightness step per period.
 */
void __interrupt(irq(TMR2), base(IVT_BASE), low_priority) TMR2_ISR(void) {
    TMR2IF = 0;
    pollOutputs();
}
#endif

/**
 * Each time this is called we increase the global brightness value. When brightness 
 * reaches max it is reset back to 0, this also moves to the next display row.
 * Starting with all enabled LEDs on then any LEDs which have a brightness setting 
 * less than the global brightness are turned off.
 * 
 * A complete refresh of the LED matrix takes MAX_BRIGHTNESS * NUM_ROWS = 32*4 = 128
 * calls of this function.
 * To get a 50Hz refresh rate this function must be called at least every 156us.
 *  
 * It is called from the TMR2 interrupt every LED_MATRIX_ISR_PERIOD_US. When it
 * was called from the poll loop the length of each step, and so the LED duty
 * cycle, depended on how long the loop took: while MMC was backing the module
 * up every message handled stretched a step and the LEDs dimmed and pulsed.
 * Without LED_MATRIX_ISR it is called from the poll loop as before.
 */
void pollOutputs(void)
{
    uint8_t i;
    
    if (brightness == 0) {
        // move to next row
        current_row++;
        current_row &= NUM_LED_ROWS-1;     // NUM_LED_ROWS is a power of 2

        // disable the cathode driver
        LAT_TLC5917_13 = 1; // OE
        // also turn the anodes off
        LAT_LED_ROW_1 = 0;
        LAT_LED_ROW_2 = 0;
        LAT_LED_ROW_3 = 0;
        LAT_LED_ROW_4 = 0;

        cathodes = ledMatrix[current_row];
        rowBright = ledBright + current_row * NUM_LED_COLUMNS;
        SPI1TCNTH=0;     // 1 byte
        SPI1TCNTL=1;     // 1 byte
        SPI1TWIDTH=0;   // 8 bits

        SPI1TXB = cathodes; // do the write and send the data

        // wait for data to be sent and latched to form the anode outputs
        // This takes quite a few cycles but we have to ensure the cathodes have the
        // right data before turning on the anodes otherwise we don't get a clean display.
        while (! SPI1STATUSbits.TXBE)
            ;
        // It can take up to 365ns for the TLC5917 outputs to output correct data (LE to OUT)
        // This is 5 instruction cycles (5 * 62.5 ns)
        // following code before enabling the cathodes is more than 5 instructions

        // turn the relevant anode driver on
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
        // Same row but turn off any LEDs that have their brightness setting
        // less than the current brightness. We do NOT change the anodes here.
        // Work out the new pattern first, using a rolling mask and a walking
        // pointer, and only blank the outputs and send it if it has changed:
        // most steps change nothing.
        uint8_t newCathodes = cathodes;
        uint8_t mask = 1;
        uint8_t *rb = rowBright;
        for (i=0; i<8; i++) {
            if (brightness > *rb++) {
                newCathodes &= (uint8_t)~mask;
            }
            mask <<= 1;
        }
        if (newCathodes != cathodes) {
            cathodes = newCathodes;
            // disable the cathode driver
            LAT_TLC5917_13 = 1; // OE
            SPI1TXB = cathodes; // do the write and send the data
            // This takes quite a few cycles but we have to ensure the cathodes have the
            // right data before latching cathodes.
            while (! SPI1STATUSbits.TXBE)
                ;
            // enable the cathode driver
            LAT_TLC5917_13 = 0; //OE
        }
    }
    brightness += 2;
    brightness &= MAX_BRIGHTNESS-1;     // wrap to 0 at MAX_BRIGHTNESS
}


/**
 * Turn on an LED. No is 0-31.
 * @param no
 */
void setLed(uint8_t no) {
    ledMatrix[no/8] |= bitMask[no & 7];
}

/**
 * Turn an LED off. No is 0-31
 */
void clearLed(uint8_t no) {
    ledMatrix[no/8] &= (uint8_t)~bitMask[no & 7];
}

/**
 * Test if an LED is on.
 * @param no
 * @return 0 if OFF or non zero if ON
 */
uint8_t testLed(uint8_t no) {
    return ledMatrix[no/8] & bitMask[no & 7];
}

