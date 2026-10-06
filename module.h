#ifndef _MODULE_H_
#define _MODULE_H_

#define RESUME_OPT      2   // KeithB b14-25: for #pragma opt in pollOutputsOld()
// comment out for CBUS
#define VLCB
// Enable FCU compatibility
#define FCU_COMPAT

// KeithB b57: TICK_ONCE_PER_PASS removed - the keithb library always reads the tick once per
// main-loop pass (tickNow), so the option no longer exists.

#include "statusLeds.h"

//
// VLCB Service options first
//
// The data version stored at NV#0
#define APP_NVM_VERSION 1
// KeithB b57: switch and LED states are buffered in RAM and written to EEPROM in the
// background by the library (as upstream keithb), replacing CANPAN3's own EEPROMbuffer.c.
// The buffer covers EEPROM_BASE_ADDRESS..+NUMBER_EEPROM (below); other EEPROM is written directly.
#define ASYNC_EEPROM    BUFFER
// KeithB b57: wait for Vdd at power-up instead of a fixed ~1 s (as upstream keithb).
// 0x0B = HLVD 3.64/4.00/4.36 V min/typ/max on the Q83.
#define VLCB_VDD_GUARD  0x0B
// Also refuse EEPROM/flash writes while Vdd is below that level (keithb 4b2c3eb).
//#define VLCB_VDD_WRITE_GUARD

#if defined(_18FXXQ83_FAMILY_)
#define IVT_BASE      0x900
#define IVT_BASE_U    0x00
#define IVT_BASE_H    0x09
#define IVT_BASE_L    0x00
#endif

//
// NV service
//
#define NV_ADDRESS      0x200
#define NV_NVM_TYPE     EEPROM_NVM_TYPE

#define NV_CACHE

//
// CAN service
//
#define CANID_ADDRESS           0x3FE    // 1 byte
#define CANID_NVM_TYPE          EEPROM_NVM_TYPE
#define CAN_INTERRUPT_PRIORITY  0    // all low priority
#define CAN_CLOCK_MHz           64
// Number of buffers
#if defined(_18F66K80_FAMILY_)
#define CAN_NUM_RXBUFFERS   32  
#define CAN_NUM_TXBUFFERS   8
#endif
#if defined(_18FXXQ83_FAMILY_)
#define CAN_NUM_RXBUFFERS   8
#endif
// KeithB b38: after a factory reset (CANID 0) enumerate before the first transmission
// instead of transmitting straight away as CANID 1. Comment out to restore upstream behaviour.
// keithb 7224ef1 renamed the option (was CAN_ENUM_BEFORE_FIRST_TX).
#define CAN_ADDITIONAL_CANID_CHECKS
// KeithB b39 CAN_RESERVE_MSG_RAM removed: keithb 5c12aa2 took the reservation out of the
// library as not needed, so the option no longer exists.
//
// BOOT service
//
#define BOOT_FLAG_ADDRESS   0x3FF
#define BOOT_FLAG_NVM_TYPE EEPROM_NVM_TYPE
#define BOOTLOADER_PRESENT

// Hardware types really determines the Pushbutton ports and LED ports
// The HARDWARE define must be set within the MPLAB project settings for a
// particular bootloader config then that config can be added to the module's project
#define HW_CANSCAN      1
#define HW_CANDISP      2
#define HW_CANPAN3      3
    
// FLiM Pushbutton, status LEDs and other module specific pins
/*
 * These definitions are required by the FLiM library code
 */

// KeithB b14-25: per-hardware section, was the fixed CANPAN3 set
#if HARDWARE==HW_CANPAN3
#define NV_NUM          67
#define NUM_SERVICES    8
#define SetPortDirections(){WPUA=0b00001000;TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA3=1;}   // KeithB b34: no RA5 pull-up (upstream 5a13)
//#define FLiM_SW         PORTAbits.RA3
//#define LED1Y           LATBbits.LATB6  // Yellow LED
//#define LED2G           LATBbits.LATB7  // Green LED
#define TRIS_LED1Y      TRISBbits.TRISB6
#define TRIS_LED2G      TRISBbits.TRISB7
// LEDs and PB                                 // GREEN is 0 YELLOW is 1
#if defined(_18F66K80_FAMILY_)
    #define APP_setPortDirections(){ANCON0=ANCON1=0; TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA2=1;}
#endif
#if defined(_18FXXQ83_FAMILY_)
    #define APP_setPortDirections(){ANSELA=ANSELB=0; WPUA=0b00001000;TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA3=1;}   // KeithB b34: no RA5 pull-up (upstream 5a13)
#endif
#define APP_writeLED1(state)   do{ if (state) LATBbits.LATB7 = 1; else LATBbits.LATB7 = 0; }while(0) /* KeithB b29: atomic BSF/BCF, safe against the LED matrix ISR */   // GREEN true is on
#define APP_writeLED2(state)   do{ if (state) LATBbits.LATB6 = 1; else LATBbits.LATB6 = 0; }while(0) /* KeithB b29: atomic BSF/BCF, safe against the LED matrix ISR */   // YELLOW true is on 
#define APP_pbPressed()        (!(PORTAbits.RA3))       // where the push button is connected. True when pressed

#define PARAM_MODULE_ID         MTYP_CANPAN
// Module name - must be 7 characters
#define NAME    "PAN    "
#define EVperEVT            13  // number of EVs per event
#define PARAM_NUM_EV_EVENT  EVperEVT
#define NUM_BUTTON_ROWS     4
#define NUM_BUTTON_COLUMNS  8
#define NUM_LED_ROWS        4
#define NUM_LED_COLUMNS     8

// CANPAN3, CANDISP and CANSCAN pin assignment
#define TRIS_LED_ROW_1      TRISBbits.TRISB4
#define TRIS_LED_ROW_2      TRISBbits.TRISB5
#define TRIS_LED_ROW_3      TRISCbits.TRISC6
#define TRIS_LED_ROW_4      TRISCbits.TRISC7

#define LAT_LED_ROW_1       LATBbits.LATB4
#define LAT_LED_ROW_2       LATBbits.LATB5
#define LAT_LED_ROW_3       LATCbits.LATC6
#define LAT_LED_ROW_4       LATCbits.LATC7

#define TRIS_TLC5917__2     TRISCbits.TRISC5
#define TRIS_TLC5917__3     TRISCbits.TRISC3
#define TRIS_TLC5917__4     TRISCbits.TRISC4
#define TRIS_TLC5917_13     TRISCbits.TRISC2

#define LAT_TLC5917__2      LATCbits.LATC5
#define LAT_TLC5917__3      LATCbits.LATC3
#define LAT_TLC5917__4      LATCbits.LATC4
#define LAT_TLC5917_13      LATCbits.LATC2

// CANPAN3 and CANSCAN pin assignment
#define TRIS_74HC238_1      TRISAbits.TRISA0
#define TRIS_74HC238_2      TRISAbits.TRISA1
#define TRIS_74HC238_3      TRISAbits.TRISA2
// Not used on CANDISP or CANPAN3
#define TRIS_74HC238_4_6    TRISAbits.TRISA?

// CANPAN3 and CANSCAN pin assignment
#define LAT_74HC238_1       LATAbits.LATA0
#define LAT_74HC238_2       LATAbits.LATA1
#define LAT_74HC238_3       LATAbits.LATA2
// Not used on CANDISP or CANPAN3
#define LAT_74HC238_4_6     LATAbits.LATA?

// CANPAN3 and CANSCAN pin assignment
#define TRIS_Srow_1         TRISBbits.TRISB0
#define TRIS_Srow_2         TRISBbits.TRISB1
#define TRIS_Srow_3         TRISCbits.TRISC0
#define TRIS_Srow_4         TRISCbits.TRISC1

// CANPAN3 and CANSCAN pin assignment
#define LAT_Srow_1          LATBbits.LATB0
#define LAT_Srow_2          LATBbits.LATB1
#define LAT_Srow_3          LATCbits.LATC0
#define LAT_Srow_4          LATCbits.LATC1

// KeithB b14-25: CANDISP - 64 LEDs, two TLC5917, no switches
#elif HARDWARE==HW_CANDISP
#define NUM_SERVICES        6
#define SetPortDirections(){TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA2=1;}
//#define FLiM_SW             PORTAbits.RA2
//#define LED1Y               LATBbits.LATB6  // Yellow LED
//#define LED2G               LATBbits.LATB7  // Green LED
#define TRIS_LED1Y          TRISBbits.TRISB6
#define TRIS_LED2G          TRISBbits.TRISB7
// LEDs and PB                                 // GREEN is 0 YELLOW is 1
#if defined(_18F66K80_FAMILY_)
    #define APP_setPortDirections(){ANCON0=ANCON1=0; TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA2=1;}
#endif
#if defined(_18FXXQ83_FAMILY_)
    #define APP_setPortDirections(){ANSELA=ANSELB=0; WPUA=0b00100100;TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA2=1;}
#endif
#define APP_writeLED1(state)   do{ if (state) LATBbits.LATB7 = 1; else LATBbits.LATB7 = 0; }while(0) /* KeithB b29: atomic BSF/BCF, safe against the LED matrix ISR */   // GREEN true is on
#define APP_writeLED2(state)   do{ if (state) LATBbits.LATB6 = 1; else LATBbits.LATB6 = 0; }while(0) /* KeithB b29: atomic BSF/BCF, safe against the LED matrix ISR */   // YELLOW true is on 
#define APP_pbPressed()        (!(PORTAbits.RA2))       // where the push button is connected. True when pressed
	
#define PARAM_MODULE_ID     MTYP_CANDISP
#define NV_NUM          67
// Module name - must be 7 characters
#define NAME                "DISP   "
#define EVperEVT            21
#define PARAM_NUM_EV_EVENT  EVperEVT
#define NUM_BUTTON_ROWS     0
#define NUM_BUTTON_COLUMNS  0
#define NUM_LED_ROWS        4
#define NUM_LED_COLUMNS     16

// CANPAN3, CANDISP and CANSCAN pin assignment
#define TRIS_LED_ROW_1      TRISBbits.TRISB0
#define TRIS_LED_ROW_2      TRISBbits.TRISB1
#define TRIS_LED_ROW_3      TRISBbits.TRISB4
#define TRIS_LED_ROW_4      TRISBbits.TRISB5

#define LAT_LED_ROW_1       LATBbits.LATB0
#define LAT_LED_ROW_2       LATBbits.LATB1
#define LAT_LED_ROW_3       LATBbits.LATB4
#define LAT_LED_ROW_4       LATBbits.LATB5

#define TRIS_TLC5917__2     TRISCbits.TRISC5
#define TRIS_TLC5917__3     TRISCbits.TRISC3
#define TRIS_TLC5917__4     TRISCbits.TRISC4
#define TRIS_TLC5917_13     TRISCbits.TRISC2

#define LAT_TLC5917__2      LATCbits.LATC5
#define LAT_TLC5917__3      LATCbits.LATC3
#define LAT_TLC5917__4      LATCbits.LATC4
#define LAT_TLC5917_13      LATCbits.LATC2

// CANPAN3 and CANSCAN pin assignment
#define TRIS_74HC238_1      TRISAbits.TRISA0
#define TRIS_74HC238_2      TRISAbits.TRISA1
#define TRIS_74HC238_3      TRISAbits.TRISA2
// Not used on CANDISP or CANPAN3
#define TRIS_74HC238_4_6    TRISAbits.TRISA? 

// CANPAN3 and CANSCAN pin assignment
#define LAT_74HC238_1       LATAbits.LATA0
#define LAT_74HC238_2       LATAbits.LATA1
#define LAT_74HC238_3       LATAbits.LATA2
// Not used on CANDISP or CANPAN3
#define LAT_74HC238_4_6     LATAbits.LATA?

// CANPAN3 and CANSCAN pin assignment
#define TRIS_Srow_1         TRISCbits.TRISC6
#define TRIS_Srow_2         TRISCbits.TRISC7
#define TRIS_Srow_3         TRISCbits.TRISC0
#define TRIS_Srow_4         TRISCbits.TRISC1

// CANPAN3 and CANSCAN pin assignment
#define LAT_Srow_1          LATCbits.LATC6
#define LAT_Srow_2          LATCbits.LATC7
#define LAT_Srow_3          LATCbits.LATC0
#define LAT_Srow_4          LATCbits.LATC1

// KeithB b14-25: CANSCAN - 128 switches, no LEDs
#elif HARDWARE==HW_CANSCAN
#define NV_NUM              131
#define NUM_SERVICES        8
#define SetPortDirections(){TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA5=1;}
//#define FLiM_SW             PORTAbits.RA5
//#define LED1Y               LATBbits.LATB6  // Yellow LED
//#define LED2G               LATBbits.LATB7  // Green LED
#define TRIS_LED1Y          TRISBbits.TRISB6
#define TRIS_LED2G          TRISBbits.TRISB7
// LEDs and PB                                 // GREEN is 0 YELLOW is 1
#if defined(_18F66K80_FAMILY_)
    #define APP_setPortDirections(){ANCON0=ANCON1=0; TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA5=1;}
#endif
#if defined(_18FXXQ83_FAMILY_)
    #define APP_setPortDirections(){ANSELA=ANSELB=0; WPUA=0b00100000;TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA5=1;}
#endif
#define APP_writeLED1(state)   do{ if (state) LATBbits.LATB7 = 1; else LATBbits.LATB7 = 0; }while(0) /* KeithB b29: atomic BSF/BCF, safe against the LED matrix ISR */   // GREEN true is on
#define APP_writeLED2(state)   do{ if (state) LATBbits.LATB6 = 1; else LATBbits.LATB6 = 0; }while(0) /* KeithB b29: atomic BSF/BCF, safe against the LED matrix ISR */   // YELLOW true is on 
#define APP_pbPressed()        (!(PORTAbits.RA5))       // where the push button is connected. True when pressed
#define PARAM_MODULE_ID     MTYP_CANSCAN
// Module name - must be 7 characters
#define NAME                "SCAN   "
#define EVperEVT            4  // number of EVs per event
#define PARAM_NUM_EV_EVENT  EVperEVT
#define NUM_BUTTON_COLUMNS  16
#define NUM_BUTTON_ROWS     8
#define NUM_LED_ROWS        0
#define NUM_LED_COLUMNS     8       // 8 rather than 0 to avoid a divide by 0 warning
// Not used on CANSCAN code.
#define TRIS_LED_ROW_1      TRISBbits.TRISB0 //->X
#define TRIS_LED_ROW_2      TRISBbits.TRISB1 //->X
#define TRIS_LED_ROW_3      TRISBbits.TRISB4 //->X
#define TRIS_LED_ROW_4      TRISBbits.TRISB5 //->X

#define LAT_LED_ROW_1       LATBbits.LATB0 //->X
#define LAT_LED_ROW_2       LATBbits.LATB1 //->X
#define LAT_LED_ROW_3       LATBbits.LATB4 //->X
#define LAT_LED_ROW_4       LATBbits.LATB5 //->X

#define TRIS_TLC5917__2     TRISCbits.TRISC5 // Repoint
#define TRIS_TLC5917__3     TRISCbits.TRISC3 // Repoint
#define TRIS_TLC5917__4     TRISCbits.TRISC4 // Repoint
#define TRIS_TLC5917_13     TRISCbits.TRISC2 // Repoint

#define LAT_TLC5917__2      LATCbits.LATC5 // Repoint
#define LAT_TLC5917__3      LATCbits.LATC3 // Repoint
#define LAT_TLC5917__4      LATCbits.LATC4 // Repoint
#define LAT_TLC5917_13      LATCbits.LATC2 // Repoint
// Used on CANSCAN
#define TRIS_74HC238_1      TRISAbits.TRISA0
#define TRIS_74HC238_2      TRISAbits.TRISA1
#define TRIS_74HC238_3      TRISAbits.TRISA2
#define TRIS_74HC238_4_6    TRISAbits.TRISA3

#define LAT_74HC238_1       LATAbits.LATA0
#define LAT_74HC238_2       LATAbits.LATA1
#define LAT_74HC238_3       LATAbits.LATA2
#define LAT_74HC238_4_6     LATAbits.LATA3

#define TRIS_Srow_1         TRISCbits.TRISC0
#define TRIS_Srow_2         TRISCbits.TRISC1
#define TRIS_Srow_3         TRISCbits.TRISC2
#define TRIS_Srow_4         TRISCbits.TRISC3
#define TRIS_Srow_5         TRISCbits.TRISC4
#define TRIS_Srow_6         TRISCbits.TRISC5
#define TRIS_Srow_7         TRISCbits.TRISC6
#define TRIS_Srow_8         TRISCbits.TRISC7

#define LAT_Srow_1          LATCbits.LATC0
#define LAT_Srow_2          LATCbits.LATC1
#define LAT_Srow_3          LATCbits.LATC2
#define LAT_Srow_4          LATCbits.LATC3
#define LAT_Srow_5          LATCbits.LATC4
#define LAT_Srow_6          LATCbits.LATC5
#define LAT_Srow_7          LATCbits.LATC6
#define LAT_Srow_8          LATCbits.LATC7

#else
// KeithB b14-25
#error You must define a HARDWARE setting in the MPLABX XC8 Defines configuration
#endif

// KeithB b26-b32: LED matrix driver options
#if HARDWARE==HW_CANPAN3 || HARDWARE==HW_CANDISP
//
// LED matrix refresh (CANPAN3 / CANDISP)
// Define LED_MATRIX_ISR to drive pollOutputs() from a TMR2 interrupt every
// LED_MATRIX_ISR_PERIOD_US microseconds, giving a constant PWM step time that
// is independent of main loop / message processing load (fixes dim/pulse
// during MMC backup). Comment out to revert to calling pollOutputs() from the
// main loop.
//
#define LED_MATRIX_ISR
#define LED_MATRIX_ISR_PERIOD_US    100
// KeithB b40: rolling-mask version of the per-step dimming loop in pollOutputs()
// (~5us instead of ~20us of each 100us ISR period). Comment out to use the original loop.
#define LED_FAST_DIM
// Dead time in microseconds at each row change, between switching the
// previous row's anode off and enabling the next row's cathodes. Stops the
// previous row ghosting (dim "off" LEDs) while its anode driver is still
// turning off. Costs LED_ROW_BLANK_US/(16 steps * step time) of brightness,
// e.g. 10us of 1600us = 0.6%. Comment out to disable.
//#define LED_ROW_BLANK_US            10
// Software ghost suppression at each row change. The row lines (TBD62783
// source outputs) and column lines (TLC5917 sink outputs) have no discharge
// path when off, so the charge they hold is dumped through whichever LEDs
// happen to connect a charged line to a sinking column at the next row -
// dim "off" LEDs. With this defined the row change is sequenced so that the
// stored charge is steered through LEDs that are lit anyway:
//   1. previous anode still on, sinks off: columns that were sinking charge
//      up through the previous row's lit LEDs.
//   2. anode off, sinks on for the LEDs lit in BOTH previous and next row:
//      the previous row line discharges through LEDs that stay lit.
//   3. next row data latched, next anode on, sinks on.
// Rows with no lit LEDs are never energised.
// Comment out for the plain row change.
//#define LED_GHOST_SUPPRESS
#define LED_GHOST_PRECHARGE_US      2   // step 1 settle time
#define LED_GHOST_DISCHARGE_US      4   // step 2 settle time
// Wait for the SPI transfer to the TLC5917 by polling TXBE (as the original
// spikyian code does) rather than BUSY. The TLC5917 latch is transparent
// while LE (driven by SPI SS) is high, so the outputs follow the shift
// register bit by bit during a transfer. BUSY clears before the last bits
// have physically clocked out, so OE was being re-enabled while the data
// was still rippling through the outputs: every LED in the row got a
// ~250ns flash per transfer, 16 transfers per row = a dim glow on LEDs
// that should be off. TXBE only sets once the byte is fully out.
// Leave defined. Comment out only to reproduce the ghosting for comparison.
#define LED_SPI_WAIT_TXBE
// DIAGNOSTIC: skip the per-step dimming transfers entirely. Each row is
// then written once at the row change and left alone for its 16 steps
// (all LEDs at full brightness, NV brightness ignored). If the ghost goes
// away with this defined, it is caused by the 16 OE-off/SPI/OE-on cycles
// within a row; if it stays, it is caused by the row change itself.
//#define LED_DIAG_NO_DIMMING
#endif

//
// EVENT TEACH SERVICE
//
//
// KeithB b14-25: EVperEVT is per hardware
#define EVENT_TABLE_WIDTH   EVperEVT    // This the the width of the table - not the 
                                        // number of EVs per event as multiple rows in
                                        // the table can be used to store an event
#define NUM_EVENTS          254         // The number of rows in the event table. The
                                        // actual number of events may be less than this
                                        // if any events use more the 1 row.
#define EV_FILL             0
#define NO_ACTION           0
#define EVENT_HASH_TABLE
#define EVENT_HASH_LENGTH   32
#define EVENT_CHAIN_LENGTH  20

#if defined(_18FXXQ83_FAMILY_)
    // KeithB b14-25: was 0x1E800, moved down so CANDISP events fit
    #define EVENT_TABLE_ADDRESS               0x1E000
#endif
#if defined(_18F66K80_FAMILY_)
    #ifdef __18F25K80
        #define EVENT_TABLE_ADDRESS               0x6E00      //(AT_NV - sizeof(EventTable)*NUM_EVENTS) Size=256 * 22 = 5632(0x1600) bytes
    #endif
    #ifdef __18F26K80
        #define EVENT_TABLE_ADDRESS       0xEE00      //(AT_NV - sizeof(EventTable)*NUM_EVENTS) Size=256 * 22 = 5632(0x1600) bytes   
    #endif
#endif

#define EVENT_TABLE_NVM_TYPE    FLASH_NVM_TYPE
// KeithB b54: at power-up, clear event rows left erased (flags and EN 0xFF) by a power cut
// between a flash page erase and its write. Comment out to leave the table as found.
#define EVENT_TABLE_HEAL_ERASED
#define CONSUMED_EVENTS
//
// EVENT PRODUCER SERVICE
#define PRODUCED_EVENTS
#define HAPPENING_SIZE      1
#define MAX_HAPPENINGS      32      // Need 128 for CANSCAN but doesn't appear to be used?
#define HAPPENING_BASE      2
//
// EVENT CONSUMER SERVICE
#define HANDLE_DATA_EVENTS

//
// MNS service
//
// Processor clock speed
#define clkMHz                  64
// 2 bytes for the module's node number
#define NN_ADDRESS              0x3FC 
#define NN_NVM_TYPE             EEPROM_NVM_TYPE
// 1 byte for the version number
#define VERSION_ADDRESS         0x3FA
#define VERSION_NVM_TYPE        EEPROM_NVM_TYPE
// 1 byte for the mode
#define MODE_ADDRESS            0x3FB
#define MODE_NVM_TYPE           EEPROM_NVM_TYPE
// 1 byte for the mode flags
#define MODE_FLAGS_ADDRESS      0x3F9
#define MODE_FLAGS_NVM_TYPE     EEPROM_NVM_TYPE
// Parameters
#define PARAM_MANU              MANU_MERG

#define PARAM_MAJOR_VERSION     5
#define PARAM_MINOR_VERSION     'a'
#define PARAM_BUILD_VERSION     57

#define PARAM_NUM_NV            NV_NUM
#define PARAM_NUM_EVENTS        NUM_EVENTS

// enable this for additional validation checks
//#define SAFETY

// Module specific stuff here

#define NUM_BUTTONS         (NUM_BUTTON_COLUMNS*NUM_BUTTON_ROWS)
#define NUM_BUTTON_BYTES    (NUM_BUTTONS/8)   // KeithB b14-25

#define NUM_PRODUCED_EVENTS (NUM_BUTTONS+1)     // +1 for the auto generated SoD
#define SOD_PSEUDO_SWITCH   (NUM_BUTTONS+1)

#define NUM_LEDS            (NUM_LED_ROWS*NUM_LED_COLUMNS)
#define NUM_LED_BYTES       (NUM_LEDS/8)   // KeithB b14-25

// Store the Switches at 0x0000 followed by the LEDs at 0x00020
#define EEPROM_BASE_ADDRESS 0x0000
// Must be a power of two, e.g. 32, 64, 128
#define NUMBER_EEPROM       (NUM_BUTTONS + NUM_LEDS)   // KeithB b14-25: was 64
_Static_assert((NUMBER_EEPROM & (NUMBER_EEPROM-1)) == 0, "NUMBER_EEPROM: Must be power of two");

#endif