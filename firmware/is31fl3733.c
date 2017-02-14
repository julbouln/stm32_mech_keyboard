/*
ISSI IS31FL3733 matrix controller
*/
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/i2c.h>
#include <libopencm3/stm32/gpio.h>

#include "is31fl3733.h"

void i2c_setup()
{
    i2c_reset(I2C1);

    /* Set alternate functions for the SCL and SDA pins of I2C1. */
    gpio_mode_setup(GPIOB, GPIO_MODE_AF, GPIO_PUPD_NONE, GPIO6 | GPIO7);
    gpio_set_af(GPIOB, GPIO_AF1, GPIO6 | GPIO7);

    /* Disable the I2C before changing any configuration. */
    i2c_peripheral_disable(I2C1);

    i2c_enable_analog_filter(I2C1);
    i2c_set_digital_filter(I2C1, I2C_CR1_DNF_DISABLED);

    /* I2C clocked by default from HSI (8MHz) */
    /* can be configured to use SYSCLK: RCC_CFGR3 |= (1<<4); */

    /* The timing settings for I2C are all set in I2C_TIMINGR; */
    /* There's a xls for computing it: STSW-STM32126 AN4235 (For F0 and F3) */

    // I2C2_TIMINGR = 0x00301D2B;

    i2c_100khz_i2cclk8mhz(I2C1);

    // 10khz
    /*	i2c_set_prescaler(I2C1, 1);
	    i2c_set_scl_low_period(I2C1, 0xC7);
	    i2c_set_scl_high_period(I2C1, 0xC3);
	    i2c_set_data_hold_time(I2C1, 0x2);
	    i2c_set_data_setup_time(I2C1, 0x4);
	*/
    i2c_enable_stretching(I2C1);
    //addressing mode
    i2c_set_7bit_addr_mode(I2C1);

    /* If everything is configured -> enable the peripheral. */
    i2c_peripheral_enable(I2C1);
}

void dotsmatrix_setup()
{
    // enable pin
    gpio_mode_setup(DOTSMATRIX_SDB_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, DOTSMATRIX_SDB_PIN);
    gpio_set(DOTSMATRIX_SDB_PORT, DOTSMATRIX_SDB_PIN);

    i2c_setup();

	delay(100);
	// software enable
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG3);
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTMATRIX_PG3_CFG, DOTMATRIX_PG3_CFG_SSD_EN|DOTMATRIX_PG3_CFG_BEN_EN);

	// set global current
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG3);
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTMATRIX_PG3_GCC, 0x40);

	// set pullup/down
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG3);
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTMATRIX_PG3_SW_PLUP, 0b111);

    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG3);
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTMATRIX_PG3_CS_PLDN, 0b111);


}

uint8_t dotsmatrix_read(uint8_t reg)
{
    uint8_t data[2];
    read_i2c(I2C1, DOTSMATRIX_ADDR, reg, 2, data);
    return data[0];
}

void dotsmatrix_write(uint8_t reg, uint8_t v)
{
    uint8_t data[2];
    data[0] = v;
    data[1] = 0;
    write_i2c(I2C1, DOTSMATRIX_ADDR, reg, 2, data);
}

void dotsmatrix_write_enable()
{
    dotsmatrix_write(DOTSMATRIX_CRWL, 0xC5);
}

uint16_t dotsmatrix_get_open_state(uint8_t line)
{
    uint16_t r = 0;
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG0);
    r |= dotsmatrix_read(DOTSMATRIX_PG0_OR + line * 2);
    r |= dotsmatrix_read(DOTSMATRIX_PG0_OR + line * 2 + 1) << 8;
    return r;
}

uint16_t dotsmatrix_get_short_state(uint8_t line)
{
    uint16_t r = 0;
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG0);
    r |= dotsmatrix_read(DOTSMATRIX_PG0_SR + line * 2);
    r |= dotsmatrix_read(DOTSMATRIX_PG0_SR + line * 2 + 1) << 8;
    return r;
}

uint16_t led_state[5] = {0,0,0,0,0};

void dotsmatrix_led_enable(uint8_t line, uint8_t col)
{
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG0);
    led_state[line] = led_state[line] | (1 << (col));
    dotsmatrix_write(DOTSMATRIX_PG0_OO + (line << 1) + (col >> 3), led_state[line] >> ((col >> 3) << 3));
}

void dotsmatrix_led_disable(uint8_t line, uint8_t col)
{
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG0);
    dotsmatrix_write_enable();
    led_state[line] = led_state[line] & ~(1 << (col));
    dotsmatrix_write(DOTSMATRIX_PG0_OO + (line << 1) + (col >> 3), led_state[line] >> ((col >> 3) << 3));
}

void dotsmatrix_led_set_pwm(uint8_t line, uint8_t col, uint8_t v)
{
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG1);
    dotsmatrix_write((line << 4) + col, v);
}

void dotsmatrix_led_set_breath(uint8_t line, uint8_t col, uint8_t v)
{
    dotsmatrix_write_enable();
    dotsmatrix_write(DOTSMATRIX_CR, DOTSMATRIX_CR_PG2);
    dotsmatrix_write((line << 4) + col, v);
}