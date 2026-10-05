#ifndef _MODULE_H_
#define _MODULE_H_

// comment out for CBUS
#define VLCB
// Enable FCU compatibility
#define FCU_COMPAT

#include "statusLeds.h"

//
// VLCB Service options first
//
// The data version stored at NV#0
#define APP_NVM_VERSION 1
#define NUM_SERVICES 8
#define ASYNC_EEPROM    BUFFER


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
#define CANID_ADDRESS  0x3FE    // 1 byte
#define CANID_NVM_TYPE EEPROM_NVM_TYPE
#define CAN_INTERRUPT_PRIORITY 0    // all low priority
#define CAN_CLOCK_MHz   64
// Number of buffers
#if defined(_18F66K80_FAMILY_)
#define CAN_NUM_RXBUFFERS   32  
#define CAN_NUM_TXBUFFERS   8
#endif
#if defined(_18FXXQ83_FAMILY_)
#define CAN_NUM_RXBUFFERS   8
#endif
// After a factory reset (CANID 0) enumerate before the first transmission instead
// of transmitting straight away as CANID 1, and never send a frame as CANID 0.
#define CAN_ADDITIONAL_CANID_CHECKS
//
// BOOT service
//
#define BOOT_FLAG_ADDRESS   0x3FF
#define BOOT_FLAG_NVM_TYPE EEPROM_NVM_TYPE
#define BOOTLOADER_PRESENT

//
// EVENT TEACH SERVICE
//
//
#define EV_FILL             0
#define NO_ACTION           0
#define EVENT_HASH_TABLE
#define EVENT_HASH_LENGTH   32
#define EVENT_CHAIN_LENGTH  20

#if defined(_18FXXQ83_FAMILY_)
    #define EVENT_TABLE_ADDRESS               0x1E800
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
// At power-up clear any event row left erased (flags and EN all 0xFF) by a power
// cut between a flash page erase and its write.
#define EVENT_TABLE_HEAL_ERASED
#define CONSUMED_EVENTS
//
// EVENT PRODUCER SERVICE
#define PRODUCED_EVENTS
#define HAPPENING_SIZE      1
#define MAX_HAPPENINGS      32
#define HAPPENING_BASE      2
//
// EVENT CONSUMER SERVICE
#define HANDLE_DATA_EVENTS


//
// MNS service
//
// Processor clock speed
#define clkMHz      64
// 2 bytes for the module's node number
#define NN_ADDRESS  0x3FC 
#define NN_NVM_TYPE EEPROM_NVM_TYPE
// 1 byte for the version number
#define VERSION_ADDRESS    0x3FA
#define VERSION_NVM_TYPE   EEPROM_NVM_TYPE
// 1 byte for the mode
#define MODE_ADDRESS    0x3FB
#define MODE_NVM_TYPE   EEPROM_NVM_TYPE
// 1 byte for the mode flags
#define MODE_FLAGS_ADDRESS    0x3F9
#define MODE_FLAGS_NVM_TYPE   EEPROM_NVM_TYPE
// Parameters
#define PARAM_MANU              MANU_MERG

#define PARAM_MODULE_ID         MTYP_CANPAN
#define PARAM_MAJOR_VERSION     5
#define PARAM_MINOR_VERSION     'a'
#define PARAM_BUILD_VERSION     13
// Module name - must be 7 characters
#define NAME    "PAN    "

#define PARAM_NUM_NV            67
#define PARAM_NUM_EVENTS        254
#define PARAM_NUM_EV_EVENT      13

// LEDs and PB                                 // GREEN is 0 YELLOW is 1
#if defined(_18F66K80_FAMILY_)
    #define APP_setPortDirections(){ANCON0=ANCON1=0; TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA2=1;}
#endif
#if defined(_18FXXQ83_FAMILY_)
    #define APP_setPortDirections(){ANSELA=ANSELB=0; WPUA=0b00001000;TRISBbits.TRISB6=TRISBbits.TRISB7=0,TRISAbits.TRISA3=1;}
#endif
// Written as if/else so that the compiler emits a single BSF/BCF. A multi-instruction
// read-modify-write of LATB could be interrupted by the LED matrix interrupt, which
// drives the row anodes on LATB4/5, and undo its change.
#define APP_writeLED1(state)   do{ if (state) LATBbits.LATB7 = 1; else LATBbits.LATB7 = 0; }while(0)   // GREEN true is on
#define APP_writeLED2(state)   do{ if (state) LATBbits.LATB6 = 1; else LATBbits.LATB6 = 0; }while(0)   // YELLOW true is on 
#define APP_pbPressed()        (!(PORTAbits.RA3))       // where the push button is connected. True when pressed
#define VLCB_VDD_GUARD  0x0B

// enable this for additional validation checks
//#define SAFETY

// Module specific stuff here
#define NUM_BUTTON_COLUMNS  8
#define NUM_BUTTON_ROWS     4
#define NUM_BUTTONS         (NUM_BUTTON_COLUMNS*NUM_BUTTON_ROWS)

#define NUM_PRODUCED_EVENTS (NUM_BUTTONS+1)     // +1 for the auto generated SoD
#define SOD_PSEUDO_SWITCH   (NUM_BUTTONS+1)

#define NUM_LED_ROWS        4
#define NUM_LED_COLUMNS     8
#define NUM_LEDS            (NUM_LED_ROWS*NUM_LED_COLUMNS)
#define NUM_LED_BYTES       (NUM_LEDS/8)

// Pin assignments
// LED row (anode) drivers
#define TRIS_LED_ROW_1      TRISBbits.TRISB4
#define TRIS_LED_ROW_2      TRISBbits.TRISB5
#define TRIS_LED_ROW_3      TRISCbits.TRISC6
#define TRIS_LED_ROW_4      TRISCbits.TRISC7
#define LAT_LED_ROW_1       LATBbits.LATB4
#define LAT_LED_ROW_2       LATBbits.LATB5
#define LAT_LED_ROW_3       LATCbits.LATC6
#define LAT_LED_ROW_4       LATCbits.LATC7
// TLC5917 cathode driver, named by its pin numbers: 2 SDI, 3 CLK, 4 LE, 13 OE
#define TRIS_TLC5917__2     TRISCbits.TRISC5
#define TRIS_TLC5917__3     TRISCbits.TRISC3
#define TRIS_TLC5917__4     TRISCbits.TRISC4
#define TRIS_TLC5917_13     TRISCbits.TRISC2
#define LAT_TLC5917__2      LATCbits.LATC5
#define LAT_TLC5917__3      LATCbits.LATC3
#define LAT_TLC5917__4      LATCbits.LATC4
#define LAT_TLC5917_13      LATCbits.LATC2
// 74HC238 switch column decoder address inputs
#define TRIS_74HC238_1      TRISAbits.TRISA0
#define TRIS_74HC238_2      TRISAbits.TRISA1
#define TRIS_74HC238_3      TRISAbits.TRISA2
#define LAT_74HC238_1       LATAbits.LATA0
#define LAT_74HC238_2       LATAbits.LATA1
#define LAT_74HC238_3       LATAbits.LATA2
// switch row inputs
#define TRIS_Srow_1         TRISBbits.TRISB0
#define TRIS_Srow_2         TRISBbits.TRISB1
#define TRIS_Srow_3         TRISCbits.TRISC0
#define TRIS_Srow_4         TRISCbits.TRISC1

#if defined(_18FXXQ83_FAMILY_)
// Drive pollOutputs() from a TMR2 interrupt every LED_MATRIX_ISR_PERIOD_US
// microseconds, so the LED PWM step time does not depend on how long each pass
// of the main loop takes. Comment out to call pollOutputs() from loop() instead.
#define LED_MATRIX_ISR
#define LED_MATRIX_ISR_PERIOD_US    100
#endif

// Store the Switches at 0x0000 followed by the LEDs at 0x0000+NUM_BUTTONS
#define EEPROM_BASE_ADDRESS 0x0000
#define NUMBER_EEPROM       (NUM_BUTTONS + NUM_LEDS)
#endif