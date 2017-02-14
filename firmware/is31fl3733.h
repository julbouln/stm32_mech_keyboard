#ifndef IS31FL3733_H
#define IS31FL3733_H

#define DOTSMATRIX_ADDR 0x50

#define DOTSMATRIX_SDB_PORT GPIOA
#define DOTSMATRIX_SDB_PIN GPIO5

#define DOTSMATRIX_CR 0xFD // command register W
#define DOTSMATRIX_CRWL 0xFE // command register write lock R/W
#define DOTSMATRIX_IMR 0xF0 // interrupt mask register W
#define DOTSMATRIX_ISR	0xF1 // interrupt status register R

// control register pages
#define DOTSMATRIX_CR_PG0 0x00 // LED control register
#define DOTSMATRIX_CR_PG1 0x01 // PWM register
#define DOTSMATRIX_CR_PG2 0x02 // auto breath mode register
#define DOTSMATRIX_CR_PG3 0x03 // function register

// page 0
#define DOTSMATRIX_PG0_OO 0x00 // on/off register W
#define DOTSMATRIX_PG0_OR 0x18 // open register W
#define DOTSMATRIX_PG0_SR 0x30 // short register W

// page 1
#define DOTMATRIX_PG1_PWM(line,row) (line << 4)+row // PWM register 

// page 2

// page 3
#define DOTMATRIX_PG3_CFG 0x00 // configuration register

#define DOTMATRIX_PG3_CFG_SSD_EN 0b00000001 // software shutdown
#define DOTMATRIX_PG3_CFG_BEN_EN 0b00000010 // breath
#define DOTMATRIX_PG3_CFG_OSD_EN 0b00000100 // open/short

#define DOTMATRIX_PG3_GCC 0x01 // global current control register

#define DOTMATRIX_PG3_SW_PLUP 0x0F // SWy pull up
#define DOTMATRIX_PG3_CS_PLDN 0x10 // CSx pull down


void dotsmatrix_setup();
#endif