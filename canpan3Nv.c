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
#include "ticktime.h"
#include "canpan3Nv.h"
#include "canpan3Outputs.h"
#include "canpan3Leds.h"
#include "nv.h"
        
/**
 * The Application specific NV defaults are defined here.
 */
uint8_t APP_nvDefault(uint8_t index) {
    if (index < PARAM_NUM_NV) {
        // Global NVs
        if (index == NV_STARTUP)
                return 0; // Restore switches
        if (index == NV_FLASHRATE)
                return HALF_SECOND/1000;
        if (index < (NV_BRIGHTNESS + NUM_LEDS))
                return 16;
        if (index <= (NV_SWITCHMODE + NUM_BUTTONS))
                return 0x00;
        if (index == NV_STARTUP_EVENT_DELAY)
            return 0;
        // the LED logic rule NVs default to 0: every rule unused
    }
    return 0;
}

/**
 * We validate NV values here.
 * We validate the startup NV and the LED logic rule NVs.
 */
NvValidation APP_nvValidate(uint8_t index, uint8_t value)  {
    if ((index == NV_STARTUP) &&(value > 3)) {
        return INVALID;
    }
#if NUM_LOGIC_RULES > 0
    if ((index >= NV_LOGIC) && (index < NV_LOGIC + NV_LOGIC_SIZE*NUM_LOGIC_RULES)) {
        switch ((uint8_t)((index - NV_LOGIC) % NV_LOGIC_SIZE)) {
            case NV_LOGIC_FUNCTION:
                if (value & LOGIC_FN_RESERVED) {
                    return INVALID;
                }
                break;
            case NV_LOGIC_INPUT_A:
            case NV_LOGIC_INPUT_B:
                // an LED, or the result of a rule
                if ((value > NUM_LEDS) &&
                        ((value <= LOGIC_INPUT_RULE) || (value > LOGIC_INPUT_RULE + NUM_LOGIC_RULES))) {
                    return INVALID;
                }
                break;
            default:
                // the true and false LEDs
                if (value > NUM_LEDS) {
                    return INVALID;
                }
                break;
        }
    }
#endif
    return VALID;
}

/**
 * We perform the necessary action when an NV changes value.
 */
void APP_nvValueChanged(uint8_t index, uint8_t value, uint8_t oldValue) {
#if NUM_LEDS > 0
    // keep the LED driver's copy of the brightness NVs up to date
    if ((index >= NV_BRIGHTNESS) && (index < NV_BRIGHTNESS + NUM_LEDS)) {
        updateLedBrightness((uint8_t)(index - NV_BRIGHTNESS), value);
    }
#endif
#if NUM_LOGIC_RULES > 0
    // show the effect of a changed logic rule straight away
    if ((index >= NV_LOGIC) && (index < NV_LOGIC + NV_LOGIC_SIZE*NUM_LOGIC_RULES)) {
        evaluateLogic();
    }
#endif
}