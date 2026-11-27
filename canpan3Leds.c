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

#if NUM_LEDS > 0    // the whole file: CANSCAN has no LEDs

//forward references
void setLedStateNoSave(uint8_t ledNo, enum canpan3LedState state);
#if NUM_LED_GROUPS > 0
static void initLedGroups(void);
#endif

static enum canpan3LedState ledStates[NUM_LEDS];
static uint8_t flashToggle;
static uint8_t startupNv;

/**
 * Initialise the LEDs.
 */
void initLeds(void) {
    uint8_t ledNo;
    
    startupNv = (uint8_t)getNV(NV_STARTUP);
    
    for (ledNo=0; ledNo<NUM_LEDS; ledNo++) {
        if (startupNv & NV_STARTUP_RESTORELEDS) {
            uint8_t state = (uint8_t)readNVM(EEPROM_NVM_TYPE, EE_ADDR_LEDS+ledNo);
            setLedStateNoSave(ledNo, (enum canpan3LedState)state);
        } else {
            ledStates[ledNo] = CANPANLED_OFF;
        }
    }
    flashToggle = 0;
#if NUM_LED_GROUPS > 0
    initLedGroups();
    evaluateLedGroups();
#endif
}

/**
 * Set the specified LED to the given state.
 * @param led
 * @param state
 */
void setLedStateNoSave(uint8_t ledNo, enum canpan3LedState state) {
    ledStates[ledNo] = state;
    switch (ledStates[ledNo]) {
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
        writeNVM(EEPROM_NVM_TYPE, EE_ADDR_LEDS+ledNo, (uint8_t)state);
    }
}

/**
 * Call regularly at required flash rate.
 */
void doFlash(void) {
    uint8_t ledNo;
    
    for (ledNo=0; ledNo<NUM_LEDS; ledNo++) {
        switch (ledStates[ledNo]) {
            case CANPANLED_FLASH:
                if (flashToggle) {
                    setLed(ledNo);
                } else {
                    clearLed(ledNo);
                }
                break;
            case CANPANLED_ANTIFLASH:
                if (flashToggle) {
                    clearLed(ledNo);
                } else {
                    setLed(ledNo);
                }
                break;
            case CANPANLED_ON:
            case CANPANLED_OFF:
                // these have already been handled in the setter above
                break;
        }
    }
    flashToggle = !flashToggle;
}

#if NUM_LED_GROUPS > 0
// LED logic groups. The NV layout is described in canpan3Nv.h.

/**
 * Clear any LED group NV with its reserved bits set. That is blank EEPROM
 * (0xFF) on a module upgraded from firmware with fewer NVs. Such a group is
 * already treated as unused, but a configuration tool that changes only some
 * of the bits would keep the reserved bits, so APP_nvValidate() would refuse
 * every change to it. Done once, at power-up.
 */
static void initLedGroups(void) {
    uint8_t group;
    
    for (group=0; group<NUM_LED_GROUPS; group++) {
        if ((uint8_t)getNV(NV_LED_GROUP + group) & GROUP_RESERVED) {
            saveNV(NV_LED_GROUP + group, 0);
        }
    }
}

/**
 * Set an LED group output. The state is not saved in EEPROM: it is worked out
 * again from the inputs at power-up.
 * @param ledNo the LED, 0..NUM_LEDS-1
 * @param on the required state
 */
static void groupOutput(uint8_t ledNo, uint8_t on) {
    enum canpan3LedState state;
    
    state = on ? CANPANLED_ON : CANPANLED_OFF;
    if (ledStates[ledNo] != state) {
        setLedStateNoSave(ledNo, state);
    }
}

/**
 * Work out every LED group in use and set its output LEDs.
 * Called after each consumed event, at power-up and when a group NV changes.
 */
void evaluateLedGroups(void) {
    uint8_t group;
    uint8_t ledNo;
    uint8_t fn;
    uint8_t a;
    uint8_t b;
    uint8_t result;
    
    ledNo = 0;
    for (group=0; group<NUM_LED_GROUPS; group++, ledNo += LED_GROUP_SIZE) {
        fn = (uint8_t)getNV(NV_LED_GROUP + group);
        if ((fn & GROUP_RESERVED) || ((fn & GROUP_FN_MASK) == GROUP_FN_NONE)) {
            continue;   // group unused (or blank EEPROM): its LEDs are left to events
        }
        // on or flashing counts as true
        a = (ledStates[ledNo] != CANPANLED_OFF);
        b = (ledStates[ledNo+1] != CANPANLED_OFF);
        if (fn & GROUP_INVERT_A) {
            a = !a;
        }
        if (fn & GROUP_INVERT_B) {
            b = !b;
        }
        switch (fn & GROUP_FN_MASK) {
            case GROUP_FN_AND:
                result = a && b;
                break;
            case GROUP_FN_OR:
                result = a || b;
                break;
            default:    // GROUP_FN_XOR
                result = (a != b);
                break;
        }
        groupOutput(ledNo+2, result);
        if ( ! (fn & GROUP_NO_FALSE_LED)) {
            groupOutput(ledNo+3, !result);
        }
    }
}
#endif  // NUM_LED_GROUPS > 0

#endif  // NUM_LEDS > 0
