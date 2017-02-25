#ifndef IS31FL3733_H
#define IS31FL3733_H

#define IS_ADDR 0x50

#define IS_SDB_PORT GPIOA
#define IS_SDB_PIN GPIO5

#define IS_CR 0xFD // command register W
#define IS_CRWL 0xFE // command register write lock R/W
#define IS_IMR 0xF0 // interrupt mask register W
#define IS_ISR	0xF1 // interrupt status register R

// control register pages
#define IS_CR_PG0 0x00 // LED control register
#define IS_CR_PG1 0x01 // PWM register
#define IS_CR_PG2 0x02 // auto breath mode register
#define IS_CR_PG3 0x03 // function register

// page 0
#define IS_PG0_OO 0x00 // on/off register W
#define IS_PG0_OR 0x18 // open register W
#define IS_PG0_SR 0x30 // short register W

// page 1
#define IS_PG1_PWM(line,row) (line << 4)+row // PWM register 

// page 2

// page 3
#define IS_PG3_CFG 0x00 // configuration register

#define IS_PG3_CFG_SSD_EN 0b00000001 // software shutdown
#define IS_PG3_CFG_BEN_EN 0b00000010 // breath
#define IS_PG3_CFG_OSD_EN 0b00000100 // open/short

#define IS_PG3_GCC 0x01 // global current control register

#define IS_PG3_SW_PLUP 0x0F // SWy pull up
#define IS_PG3_CS_PLDN 0x10 // CSx pull down

#define IS_PG3_ABC_ABM1_R1 0x02
#define IS_PG3_ABC_ABM1_R2 0x03

#define IS_PG3_ABC_T13_021S 0b00000000
#define IS_PG3_ABC_T13_042S 0b00100000
#define IS_PG3_ABC_T13_084S 0b01000000
#define IS_PG3_ABC_T13_168S 0b01100000
#define IS_PG3_ABC_T13_336S 0b10000000
#define IS_PG3_ABC_T13_672S 0b10100000
#define IS_PG3_ABC_T13_1344S 0b11000000
#define IS_PG3_ABC_T13_2688S 0b11100000

#define IS_PG3_ABC_T24_000S 0b00000000
#define IS_PG3_ABC_T24_021S 0b00000010
#define IS_PG3_ABC_T24_042S 0b00000100
#define IS_PG3_ABC_T24_084S 0b00000110
#define IS_PG3_ABC_T24_168S 0b00001000
#define IS_PG3_ABC_T24_336S 0b00001010
#define IS_PG3_ABC_T24_672S 0b00001100
#define IS_PG3_ABC_T24_1344S 0b00001110
#define IS_PG3_ABC_T24_2688S 0b00010000
#define IS_PG3_ABC_T4_5376S 0b00010001 // T4 only
#define IS_PG3_ABC_T4_10752S 0b00010010 // T4 only

#define IS_PG3_TU 0x0E // time update register


void IS_setup();
#endif