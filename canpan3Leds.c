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
#if NUM_LOGIC_RULES > 0
static void initLogic(void);
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
#if NUM_LOGIC_RULES > 0
    initLogic();
    evaluateLogic();
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

#if NUM_LOGIC_RULES > 0
// LED logic rules. The NV layout is described in canpan3Nv.h.

static uint8_t ruleResult[NUM_LOGIC_RULES];

/**
 * Clear any logic rule whose function NV has its reserved bits set. That is
 * blank EEPROM (0xFF) on a module upgraded from firmware with fewer NVs. Such a
 * rule is already treated as unused, but a configuration tool that changes only
 * the function bits would keep the reserved bits, so APP_nvValidate() would
 * refuse every change to it. Done once, at power-up.
 */
static void initLogic(void) {
    uint8_t rule;
    uint8_t nv;
    uint8_t i;

    nv = NV_LOGIC;
    for (rule=0; rule<NUM_LOGIC_RULES; rule++, nv += NV_LOGIC_SIZE) {
        if ((uint8_t)getNV(nv + NV_LOGIC_FUNCTION) & LOGIC_FN_RESERVED) {
            for (i=0; i<NV_LOGIC_SIZE; i++) {
                saveNV(nv + i, 0);
            }
        }
    }
}

/**
 * Get the value of a logic rule input.
 * @param input 0 none, 1..NUM_LEDS an LED, LOGIC_INPUT_RULE+n the result of rule n
 * @return 1 if the LED is on or flashing or the rule result is true, otherwise 0
 */
static uint8_t logicInput(uint8_t input) {
    if ((input >= 1) && (input <= NUM_LEDS)) {
        return (ledStates[input-1] != CANPANLED_OFF);
    }
    if ((input > LOGIC_INPUT_RULE) && (input <= LOGIC_INPUT_RULE + NUM_LOGIC_RULES)) {
        return ruleResult[input - LOGIC_INPUT_RULE - 1];
    }
    return 0;
}

/**
 * Drive a logic rule output LED. The state is not saved in EEPROM: it is
 * worked out again from the inputs at power-up.
 * @param led 0 none, 1..NUM_LEDS the LED
 * @param on the required state
 */
static void logicOutput(uint8_t led, uint8_t on) {
    enum canpan3LedState state;

    if ((led >= 1) && (led <= NUM_LEDS)) {
        led--;
        state = on ? CANPANLED_ON : CANPANLED_OFF;
        if (ledStates[led] != state) {
            setLedStateNoSave(led, state);
        }
    }
}

/**
 * Work out every LED logic rule, in order, and set its output LEDs.
 * Called after each consumed event, at power-up and when a rule NV changes.
 */
void evaluateLogic(void) {
    uint8_t rule;
    uint8_t nv;
    uint8_t fn;
    uint8_t inputB;
    uint8_t a;
    uint8_t b;
    uint8_t result;

    nv = NV_LOGIC;
    for (rule=0; rule<NUM_LOGIC_RULES; rule++, nv += NV_LOGIC_SIZE) {
        fn = (uint8_t)getNV(nv + NV_LOGIC_FUNCTION);
        if ((fn & LOGIC_FN_RESERVED) || ((fn & LOGIC_FN_MASK) == LOGIC_FN_NONE)) {
            // rule unused (or blank EEPROM)
            ruleResult[rule] = 0;
            continue;
        }
        a = logicInput((uint8_t)getNV(nv + NV_LOGIC_INPUT_A));
        if (fn & LOGIC_INVERT_A) {
            a = !a;
        }
        inputB = (uint8_t)getNV(nv + NV_LOGIC_INPUT_B);
        if (inputB == 0) {
            // single input: the result is input A
            result = a;
        } else {
            b = logicInput(inputB);
            if (fn & LOGIC_INVERT_B) {
                b = !b;
            }
            switch (fn & LOGIC_FN_MASK) {
                case LOGIC_FN_AND:
                    result = a && b;
                    break;
                case LOGIC_FN_OR:
                    result = a || b;
                    break;
                default:    // LOGIC_FN_XOR
                    result = (a != b);
                    break;
            }
        }
        ruleResult[rule] = result;
        logicOutput((uint8_t)getNV(nv + NV_LOGIC_TRUE_LED), result);
        logicOutput((uint8_t)getNV(nv + NV_LOGIC_FALSE_LED), !result);
    }
}
#endif  // NUM_LOGIC_RULES > 0

#endif  // NUM_LEDS > 0
