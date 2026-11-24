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

// LED logic rules (CANPAN3 and CANDISP)
//
// A rule lights one LED when a logic function of two other LEDs is true and,
// optionally, another LED when it is false. For example a white "route set" LED
// when the LEDs for both points of a section show them set, and a red LED
// otherwise. The inputs are the LEDs' current states (on or flashing counts as
// true), so the rule follows the LEDs whatever events set them.
//
// NUM_LOGIC_RULES rules (set per board in module.h) of NV_LOGIC_SIZE NVs each,
// from NV_LOGIC. Rule n (1..NUM_LOGIC_RULES) starts at
// NV_LOGIC + NV_LOGIC_SIZE*(n-1). All default to 0, which leaves the rule unused.
#define NV_LOGIC                (NV_STARTUP_EVENT_DELAY + 1)  // 68 on CANPAN3 and CANDISP
#define NV_LOGIC_SIZE           5
// NV offsets within a rule
#define NV_LOGIC_FUNCTION       0   // function and input inversion, see below
#define NV_LOGIC_INPUT_A        1   // input value, see below
#define NV_LOGIC_INPUT_B        2   // input value, see below; 0 = none, the result is input A
#define NV_LOGIC_TRUE_LED       3   // LED 1..NUM_LEDS lit while the result is true, 0 = none
#define NV_LOGIC_FALSE_LED      4   // LED 1..NUM_LEDS lit while the result is false, 0 = none
// NV_LOGIC_FUNCTION bits
#define LOGIC_FN_MASK           0x03
#define LOGIC_FN_NONE           0   // rule unused
#define LOGIC_FN_AND            1
#define LOGIC_FN_OR             2
#define LOGIC_FN_XOR            3
#define LOGIC_INVERT_A          0x04
#define LOGIC_INVERT_B          0x08
#define LOGIC_FN_RESERVED       0xF0    // must be 0, so blank EEPROM (0xFF) reads as unused
// Input values: 0 = none (false), 1..NUM_LEDS = that LED,
// LOGIC_INPUT_RULE+n = the result of rule n, so rules can be chained. A rule
// should only use the results of earlier rules: a later rule's result is the
// one from the previous evaluation.
#define LOGIC_INPUT_RULE        64

#if NUM_LOGIC_RULES > 0
#if NUM_LEDS > LOGIC_INPUT_RULE
#error LED logic: LED numbers overlap the rule result input values
#endif
#if PARAM_NUM_NV != (NV_LOGIC + NV_LOGIC_SIZE*NUM_LOGIC_RULES - 1)
#error LED logic: PARAM_NUM_NV does not match the logic rule NVs
#endif
#endif

