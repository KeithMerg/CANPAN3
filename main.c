/* TODOs

4) Save event state when EVs are edited. I'm still unsure about this so I need to 
investigate further.

5) LED flicker when a toggle switch is operated.
I can confirm this is a bug. I think this is caused by an uninterruptible delay 
caused by writing to EEPROM. If the switch processing takes place at different parts 
of the LED refresh cycle then the LED will either flash brightly or flicker more dimly. 
Still need to have a closer look on what can be done to resolve this. 
It may become a feature :)

*/

#include <xc.h>
#include "module.h"
#include "vlcb.h"
// the services
#include "mns.h"
#include "nv.h"
#include "can.h"
#include "boot.h"
#include "event_teach.h"
#include "event_consumer_simple.h"
#include "event_producer.h"
#include "event_acknowledge.h"
#include "event_coe.h"
// others
#include "ticktime.h"
#include "timedResponse.h"
#include "devincs.h"
#include <stddef.h>
#include "statusLeds.h"
#include "nvm.h"
#include "timedResponse.h"
// module specific
#include "canpan3Nv.h"
// KeithB b57: configuration words now come from the library (vlcb.c includes the template);
// including it here as well would set them twice.
#include "canpan3Inputs.h"
#include "canpan3Events.h"
#include "canpan3Outputs.h"
#include "canpan3Leds.h"

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
 *	The Main CANPAN module
 *
 * @author Ian Hogg 
 * @date August 2024
 * 
 */ 
/**
 * @copyright Creative Commons Attribution-NonCommercial-ShareAlike 4.0 International License.
 */ 




/**************************************************************************
 * Application code packed with the bootloader must be compiled with options:
 * XC8 linker options -> Additional options --CODEOFFSET=0x800 
 * This generates an error
 * ::: warning: (2044) unrecognized option "--CODEOFFSET=0x800"
 * but this can be ignored as the option works
 * 
 * Then the Application project must be made dependent on the Bootloader 
 * project by adding the Bootloader to project properties->Conf:->Loading
 ***************************************************************************/

/*
 * File:   main.c
 * Author: Ian Hogg
 * 
 * This is the main for the CANPAN3 module.
 * Based on EV and NV settings of CANPAN_4c_beta104
 */

// TODOs
// * Debounce inputs - if needed
//


// forward declarations
void __init(void);
uint8_t checkCBUS( void);
void ISRHigh(void);
void factoryResetGlobalEvents(void);
extern void initLeds(void);
extern void clearAllEvents(void);
extern uint8_t addTestEvent(uint8_t sw);
#if defined(_18F66K80_FAMILY_)
extern void inputIsr(void);
extern void outputIsr(void);
#endif

static TickValue   startTime;
static uint8_t     started;
static TickValue   lastInputScanTime;
static TickValue   flashTime;
#ifndef LED_MATRIX_ISR   // KeithB b26
static TickValue   outputPollTime;
#endif
static uint8_t     flashRateNV;   // KeithB b14-25: flash rate NV cached
static uint32_t    flashPeriod;    // KeithB b33: flash period in ticks
            
const Service * const services[] = {
    &canService,
    &mnsService,
    &nvService,
    &eventTeachService,
    &eventConsumerService,
#if HARDWARE==HW_CANSCAN || HARDWARE==HW_CANPAN3   // KeithB b14-25
    &eventProducerService,
    // KeithB b14-25: producer/COE only on switch hardware; boot service last
    &eventCoeService,
#endif
    &bootService
};


/**
 * Called at first run to initialise all the non volatile memory. 
 * Also called if the PB hold down special sequence at power up is done.
 * Also called as a result of a NNRSM request.
 */
void APP_factoryReset(void) {
    uint8_t sw;
    
    factoryResetGlobalEvents();

    flushFlashBlock();
    
#if HARDWARE==HW_CANSCAN || HARDWARE==HW_CANPAN3   // KeithB b14-25
    // Write the EEPROM for the toggle switch inputs
    for (sw=0; sw < NUM_BUTTONS; sw++) {
        writeNVM(EEPROM_NVM_TYPE, EE_ADDR_SWITCHES+sw, 0);
    }
#endif
#if HARDWARE==HW_CANPAN3 || HARDWARE==HW_CANDISP
    // KeithB b35: clear the saved LED states as well
    for (sw=0; sw < NUM_LEDS; sw++) {
        writeNVM(EEPROM_NVM_TYPE, EE_ADDR_LEDS+sw, 0);
    }
#endif
}

/**
 * Called if the PB is held down during power up.
 * Normally would perform any test functionality to help a builder check the hardware.
 * 
 * Create an event for every switch which also turns on the respective LED.
 * WARNING: Will remove any user provisioned events
 */
void APP_testMode(void) {
    uint8_t sw;
    
    clearAllEvents();
    
#if HARDWARE==HW_CANSCAN || HARDWARE==HW_CANPAN3   // KeithB b14-25
    for (sw=0; sw<NUM_BUTTONS; sw++) {
        addTestEvent(sw+1);
    }
#endif
}

/**
 * Called first thing in main(), before the power-up delay. Nothing needed.
 */
void APP_earlyInit(void) {   // KeithB b57: required by the library (as upstream keithb)
}

/**
 * Called upon power up.
 */
void setup(void) {
#if defined(_18FXXQ83_FAMILY_)
    uint8_t pu;
#endif
//    uint8_t nv;   // KeithB b14-25: unused
    
    // use CAN as the module's transport
    transport = &canTransport;

    /**
     * The order of initialisation is important.
     */
#if defined(_18F66K80_FAMILY_)
    INTCON2bits.RBPU = 0;
    // default to all digital IO
    ANCON0 = 0x00;
    ANCON1 = 0x00;
#endif
#if defined(_18FXXQ83_FAMILY_)
    // KeithB b43: pull-ups and RA5 per hardware. Upstream 5a13 is CANPAN3-only;
    // merged unconditionally (b34) it made the CANSCAN push button (RA5) an
    // output driven low, so it always read "pressed" and never released, and
    // removed the CANDISP button (RA2) pull-up.
#if HARDWARE==HW_CANSCAN
    WPUA = 0b00100000;  // ensure the pushbutton (RA5) pullup is still enabled
#elif HARDWARE==HW_CANDISP
    WPUA = 0b00100100;  // pushbutton RA2, and RA5, as APP_setPortDirections
#else
    WPUA = 0b00001000;  // ensure the pushbutton (RA3) pullup is still enabled
#endif
    WPUB = 0;
    WPUC = 0;
    ANSELA = 0x00;
    ANSELB = 0x00;
    ANSELC = 0x00;
    
    LATAbits.LATA4 = 0; TRISAbits.TRISA4 = 0;   // Unused on all three boards (pin 6: VCAP on a K80). KeithB b43: latch first
#if HARDWARE==HW_CANPAN3
    LATAbits.LATA5 = 0; TRISAbits.TRISA5 = 0;   // Unused. KeithB b34: as upstream 5a13
#endif
#endif
#if HARDWARE==HW_CANPAN3 || HARDWARE==HW_CANDISP   // KeithB b14-25
    initOutputs();
    initLeds();
    flashRateNV = (uint8_t)getNV(NV_FLASHRATE);
    flashPeriod = ((uint32_t)flashRateNV + 1) * 1000;
#endif
#if HARDWARE==HW_CANSCAN || HARDWARE==HW_CANPAN3   // KeithB b14-25
    initInputs();
#endif
    initEvents();
    
    // Lock the PPS
/*    PPSLOCK = 0x55; //Required sequence
    PPSLOCK = 0xAA; //Required sequence
    PPSLOCKbits.PPSLOCKED = 1; //Set PPSLOCKED bit
*/
    // enable interrupts, all init now done
    ei(); 

    startTime.val = tickGet();
    lastInputScanTime.val = startTime.val;
    flashTime.val = startTime.val;
#ifndef LED_MATRIX_ISR   // KeithB b26
    outputPollTime.val = startTime.val;
#endif

    started = FALSE;
    canpanScanReady = 0;
    
//    nv = (uint8_t)getNV(NV_STARTUP);
}

/**
 * The loop code call repeatedly from VLCB.
 */
void loop(void) {   // KeithB b40: tick value read once per pass (tickNowGet / tickTimeSinceNow)
    uint8_t tableIndex;
    
    // Startup delay for CBUS about 2 seconds to let other modules get powered up - ISR will be running so incoming packets processed
    if (started == FALSE) {
        if (tickTimeSinceNow(startTime) >  (TWO_SECOND+getNV(NV_STARTUP_EVENT_DELAY)*ONE_SECOND)) {
            started = TRUE;
#if HARDWARE==HW_CANSCAN || HARDWARE==HW_CANPAN3   // KeithB b14-25
            tableIndex = switch2Event[SOD_PSEUDO_SWITCH-1];
            if (tableIndex != NO_INDEX) canpanSendProducedEvent(tableIndex, TRUE);
        }
    } else {
        // KeithB b14-25: scan period from NUM_BUTTON_ROWS
        // 8/NUM_BUTTON_ROWS = 1 or 2 ms
        if (tickTimeSinceNow(lastInputScanTime) > 8/NUM_BUTTON_ROWS*ONE_MILI_SECOND) {
//        if (tickTimeSince(lastInputScanTime) > 2*ONE_MILI_SECOND) {
            inputScan();    // Strobe inputs for changes
            lastInputScanTime.val = tickNowGet();
#endif
        }
    }
#if HARDWARE==HW_CANPAN3 || HARDWARE==HW_CANDISP   // KeithB b14-25
    // KeithB b33: precomputed period, no divide per loop.
    if (tickTimeSinceNow(flashTime) >= flashPeriod) {
           // update flashing LEDs
        if(doFlash()) {
                // If we have any flashing, refresh the cached value
            flashRateNV = (uint8_t)getNV(NV_FLASHRATE);
            flashPeriod = ((uint32_t)flashRateNV + 1) * 1000;
        }
        flashTime.val = tickNowGet();
    }
#ifndef LED_MATRIX_ISR   // KeithB b26
    // poll the LED display quickly.
    if (tickTimeSinceNow(outputPollTime) > HUNDRED_MICRO_SECOND) {
        pollOutputs();
        outputPollTime.val = tickNowGet();
    }
#endif
#endif
    // KeithB b57: EEPROM writes are done by the library (ASYNC_EEPROM BUFFER): vlcb.c calls
    // pollAsyncEEPROM() on every pass of the main loop.

// KeithB b14-25: alternative loop structure, kept for reference
//    // Startup delay for CBUS about 2 seconds to let other modules get powered up - ISR will be running so incoming packets processed
//    if (started == FALSE) {
//        if (tickTimeSince(startTime) >  (TWO_SECOND+getNV(NV_STARTUP_EVENT_DELAY)*ONE_SECOND)) {
//            started = TRUE;
//#if HARDWARE==HW_CANSCAN || HARDWARE==HW_CANPAN3
//            tableIndex = switch2Event[SOD_PSEUDO_SWITCH-1];
//            if (tableIndex != NO_INDEX) canpanSendProducedEvent(tableIndex, TRUE);
//#endif
//        }
//#if HARDWARE==HW_CANPAN3 || HARDWARE==HW_CANDISP
//    // poll the LED display quickly.
//    } else if (tickTimeSince(outputPollTime) > HUNDRED_MICRO_SECOND) {
//        pollOutputs();
//        outputPollTime.val = tickGet();
//#endif
//#if HARDWARE==HW_CANSCAN || HARDWARE==HW_CANPAN3
//    } else if (tickTimeSince(lastInputScanTime) > 8/NUM_BUTTON_ROWS*ONE_MILI_SECOND) {
//        // 8/NUM_BUTTON_ROWS = 1 or 2 ms
////        if (tickTimeSince(lastInputScanTime) > 2*ONE_MILI_SECOND) {
//            inputScan();    // Strobe inputs for changes
//            lastInputScanTime.val = tickGet();
//#endif
//#if HARDWARE==HW_CANPAN3 || HARDWARE==HW_CANDISP
//    } else if (tickTimeSince(flashTime)/1000 > flashRateNV) {
//           // update flashing LEDs
//        if(doFlash()) {
//                // If we have any flashing, refresh the cached value
//            flashRateNV = (uint8_t)getNV(NV_FLASHRATE);
//        }
//        flashTime.val = tickGet();
//#endif
//    // Check to see if there are any EEPROM writes waiting to be done. 
//    // A write takes max 11 ms but CPU isn't blocked unless there is already 
//    // a write in progress. 
//    } else if (tickTimeSince(eepromWriterTime) > ONE_MILI_SECOND) {
//        pollEEPROMwriter();
//        // Keith Bruce - Added to reinstate the intended throttling.
//        eepromWriterTime.val = tickGet();
//    }
}

// Application functions required by MERGLCB library


/**
 * Check to see if now is a good time to start a flash write.
 * It is a bad time if we are currently doing a servo pulse.
 * 
 * If a servo pulse timer is currently running then the NVM routine will keep 
 * calling this until the timer expires.
 * 
 * @return GOOD_TIME if OK else BAD_TIME
 */
ValidTime APP_isSuitableTimeToWriteFlash(void){
    return GOOD_TIME;
}

/**
 * This application doesn't need to process any messages in a special way.
 */
Processed APP_postProcessMessage(Message * m) {
    return NOT_PROCESSED;
}

/**
 * This is needed by the library to get the current event state. 
 *
EventState APP_GetEventState(Happening h) {
    uint8_t flags;
    uint8_t happeningIndex;
    Boolean disable_off;
    
    
    // The TRIGGER_INVERTED has already been taken into account when saved in outputState. No need to check again
    return outputState[io]?EVENT_ON:EVENT_OFF;

}*/


#if defined(_18F66K80_FAMILY_)

// APP Interrupt service routines
void APP_lowIsr(void) {
    outputIsr();
}

// Interrupt service routines
void APP_highIsr(void) {

}
#endif
