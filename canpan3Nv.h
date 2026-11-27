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

// The possible NVs
#define NV_STARTUP              1
#define NV_FLASHRATE            2
#define NV_BRIGHTNESS           3                             // NUM_LEDS entries, 3..34 on CANPAN3
#define NV_SWITCHMODE           (NV_BRIGHTNESS+NUM_LEDS)      // NUM_BUTTONS entries, 35..66 on CANPAN3
#define NV_STARTUP_EVENT_DELAY  (NV_SWITCHMODE + NUM_BUTTONS) // 67 on CANPAN3 and CANDISP, 131 on CANSCAN

// The possible NV_STARTUP values
//#define NV_STARTUP_RESTORE  0
//#define NV_STARTUP_NOTHING  1
//#define NV_STARTUP_SCAN     2
//#define NV_STARTUP_ALLOFF   3
// Startup value bit fields
#define NV_STARTUP_RESTORESWITCHES  0x01
#define NV_STARTUP_RESTORELEDS      0x02

// Possible NV_SWITCHMODE bit values
#define NV_SWITCHMODE_PAIRED 1

// LED logic groups (CANPAN3 and CANDISP)
//
// The LEDs are taken in groups of four: LEDs 4k+1 to 4k+4 form group k+1.
// When a group is in use, its first two LEDs are the inputs, its third LED is
// lit while a logic function of them is true and its fourth LED while it is
// false. For example a white "route set" LED when the LEDs for both points of
// a section show them set, and a red LED otherwise. The inputs are the LEDs'
// current states (on or flashing counts as true), so the group follows the
// LEDs whatever events set them.
//
// One NV per group, NUM_LED_GROUPS of them (set per board in module.h) from
// NV_LED_GROUP. All default to 0, which leaves the group's four LEDs to events.
#define NV_LED_GROUP            (NV_STARTUP_EVENT_DELAY + 1)  // 68 on CANPAN3 and CANDISP
#define LED_GROUP_SIZE          4
// NV_LED_GROUP bits
#define GROUP_FN_MASK           0x03
#define GROUP_FN_NONE           0   // group unused
#define GROUP_FN_AND            1
#define GROUP_FN_OR             2
#define GROUP_FN_XOR            3
#define GROUP_INVERT_A          0x04    // invert the first LED before the function
#define GROUP_INVERT_B          0x08    // invert the second LED before the function
#define GROUP_NO_FALSE_LED      0x10    // leave the fourth LED to events
#define GROUP_RESERVED          0xE0    // must be 0, so blank EEPROM (0xFF) reads as unused

#if NUM_LED_GROUPS > 0
#if NUM_LED_GROUPS*LED_GROUP_SIZE > NUM_LEDS
#error LED logic: more groups than the LEDs allow
#endif
#if PARAM_NUM_NV != (NV_LED_GROUP + NUM_LED_GROUPS - 1)
#error LED logic: PARAM_NUM_NV does not match the LED group NVs
#endif
#endif

