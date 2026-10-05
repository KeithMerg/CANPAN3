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
 *
 * @author Ian Hogg 
 * @date October 2024
 * 
 */ 

#include <xc.h>
#include "module.h"
#include "canpan3Leds.h"
#include "canpan3Outputs.h"
#include "canpan3Nv.h"
#include "nv.h"
#include "EEPROMbuffer.h"

//forward references
void setLedStateNoSave(uint8_t ledNo, enum canpan3LedState state);

static enum canpan3LedState ledStates[NUM_LEDS ? NUM_LEDS : 1];   // KeithB b14-25: size guarded for hardware with no LEDs
static uint8_t flashToggle;
uint8_t doFlashEnabled;   // KeithB b14-25: doFlash() only runs while something is flashing
static uint8_t startupNv;

/**
 * Initialise the LEDs.
 */
void initLeds(void) {

    for (uint8_t ledNo=0; ledNo<NUM_LEDS; ledNo++) {   // KeithB b14-25
        if ((startupNv = (uint8_t) getNV(NV_STARTUP)) & NV_STARTUP_RESTORELEDS) {
            setLedStateNoSave(ledNo, (enum canpan3LedState)readEEvalue(EE_ADDR_LEDS+ledNo));
        } else {
            ledStates[ledNo] = CANPANLED_OFF;
        }
    }
    flashToggle = 0;
    doFlashEnabled = 1;
}

/**
 * Set the specified LED to the given state.
 * @param led
 * @param state
 */
void setLedStateNoSave(uint8_t ledNo, enum canpan3LedState state) {
    switch (ledStates[ledNo] = state) {
        case CANPANLED_ON:
            setLed(ledNo);
            break;
        case CANPANLED_OFF:
            clearLed(ledNo);
            break;
        case CANPANLED_FLASH:
        case CANPANLED_ANTIFLASH:
            // flashing states get dealt with by the doFlash function
            break;
    }
    
}
/**
 * Set the specified LED to the given state and save the value in EEPROM.
 * @param led
 * @param state
 */
void setLedState(uint8_t ledNo, enum canpan3LedState state) {
    setLedStateNoSave(ledNo, state);
    if (startupNv & NV_STARTUP_RESTORELEDS) {
        writeEEvalue(EE_ADDR_LEDS+ledNo, (uint8_t)state);
    }
}

/**
 * Call regularly at required flash rate.
 */
// KeithB b14-25: returns doFlashEnabled
uint8_t doFlash(void) {
    uint8_t ledNo;
    if (doFlashEnabled) {
        doFlashEnabled = 0;
        for (ledNo=0; ledNo<NUM_LEDS; ledNo++) {
            switch (ledStates[ledNo]) {
                case CANPANLED_FLASH:
                    if (flashToggle) {
                        setLed(ledNo);
                    } else {
                        clearLed(ledNo);
                    }
                    doFlashEnabled = 1;
                    break;
                case CANPANLED_ANTIFLASH:
                    if (flashToggle) {
                        clearLed(ledNo);
                    } else {
                        setLed(ledNo);
                    }
                    doFlashEnabled = 1;
                    break;
                case CANPANLED_ON:
                case CANPANLED_OFF:
                    // these have already been handled in the setter above
                    break;
            }
        }
        flashToggle = !flashToggle;
    }
    return doFlashEnabled;
}
