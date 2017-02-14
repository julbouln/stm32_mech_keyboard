
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <libopencm3/stm32/rcc.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/cm3/nvic.h>
#include <libopencm3/cm3/systick.h>
#include <libopencm3/stm32/usart.h>
#include <libopencm3/stm32/spi.h>
#include <libopencm3/stm32/dma.h>

#include <libopencm3/stm32/crs.h>
#include <libopencm3/stm32/syscfg.h>

#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/pwr.h>

#include <libopencm3/stm32/exti.h>

#include "keyboard.h"
#include "usb.h"
#include "is31fl3733.h"

#define MATRIX_ROW 5
#define MATRIX_COL 14

#define KBD_C_PORT GPIOB

#define KBD_C1 GPIO0
#define KBD_C2 GPIO1
#define KBD_C3 GPIO2
#define KBD_C4 GPIO3
#define KBD_C5 GPIO4
#define KBD_C6 GPIO5
#define KBD_C7 GPIO8
#define KBD_C8 GPIO9
#define KBD_C9 GPIO10
#define KBD_C10 GPIO11
#define KBD_C11 GPIO12
#define KBD_C12 GPIO13
#define KBD_C13 GPIO14
#define KBD_C14 GPIO15

#define KBD_R_PORT GPIOA

#define KBD_R1 GPIO0
#define KBD_R2 GPIO1
#define KBD_R3 GPIO2
#define KBD_R4 GPIO3
#define KBD_R5 GPIO4

#define KEY_NC KEY_ERROR_UNDEFINED

uint32_t kbd_col_gpio[MATRIX_COL] = {KBD_C1, KBD_C2, KBD_C3, KBD_C4, KBD_C5, KBD_C6, KBD_C7, KBD_C8, KBD_C9, KBD_C10, KBD_C11, KBD_C12, KBD_C13, KBD_C14};
uint32_t kbd_row_gpio[MATRIX_ROW] = {KBD_R1, KBD_R2, KBD_R3, KBD_R4, KBD_R5};

uint8_t kbd_matrix[MATRIX_ROW][MATRIX_COL] = {
    {KEY_ESCAPE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUALS, KEY_BACKSPACE},
    {KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEYPAD_LEFT_BRACKET, KEYPAD_RIGHT_BRACKET, KEYPAD_BACKSLASH},
    {KEY_CAPS_LOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_QUOTE, KEYPAD_BACKSLASH, KEY_ENTER},
    {KEY_LEFT_SHIFT, KEYPAD_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_NC, KEY_RIGHT_SHIFT},
    {KEY_CONTROL, KEY_NC, KEY_LEFT_ALT, KEY_NC, KEY_NC, KEY_NC, KEY_SPACE, KEY_NC, KEY_NC, KEY_NC, KEY_RIGHT_ALT, KEY_NC, KEY_NC, KEY_CONTROL},
};

volatile uint32_t system_millis;

/* Called when systick fires */
void sys_tick_handler(void)
{
    system_millis++;
}

/* sleep for delay milliseconds */
void delay(uint32_t delay)
{
    uint32_t wake = system_millis + delay;
    while (wake > system_millis)
        ;
}

/* Set up a timer to create 1mS ticks. */
static void systick_setup(void)
{
    /* clock rate / 1000 to get 1mS interrupt rate */
    systick_set_reload(48000);
    systick_set_clocksource(STK_CSR_CLKSOURCE_AHB);
    systick_counter_enable();
    /* this done last */
    systick_interrupt_enable();
}

char kbd_falling[7] = {0, 0, 0, 0, 0, 0, 0};

static void gpio_setup(void)
{
    // keyboard output
    gpio_mode_setup(KBD_C_PORT, GPIO_MODE_INPUT, GPIO_PUPD_PULLDOWN, KBD_C1 | KBD_C2 | KBD_C3 | KBD_C4 | KBD_C5 | KBD_C6 | KBD_C7 | KBD_C8 | KBD_C9 | KBD_C10 | KBD_C11 | KBD_C12 | KBD_C13 | KBD_C14);

    // keyboard input
    gpio_mode_setup(KBD_R_PORT, GPIO_MODE_OUTPUT, GPIO_PUPD_NONE, KBD_R1 | KBD_R2 | KBD_R3 | KBD_R4 | KBD_R5);

    gpio_clear(KBD_R_PORT, KBD_R1 | KBD_R2 | KBD_R3 | KBD_R4 | KBD_R5);
}

void kbd_input_scan(uint8_t *mat)
{
    //	delay(10);
    uint16_t read = gpio_port_read(KBD_C_PORT);
    mat[0] = (read >> 0) & 0b1;
    mat[1] = (read >> 1) & 0b1;
    mat[2] = (read >> 2) & 0b1;
    mat[3] = (read >> 3) & 0b1;
    mat[4] = (read >> 4) & 0b1;
    mat[5] = (read >> 5) & 0b1;
    mat[6] = (read >> 8) & 0b1;
    mat[7] = (read >> 9) & 0b1;
    mat[8] = (read >> 10) & 0b1;
    mat[9] = (read >> 11) & 0b1;
    mat[10] = (read >> 12) & 0b1;
    mat[11] = (read >> 13) & 0b1;
    mat[12] = (read >> 14) & 0b1;
    mat[13] = (read >> 15) & 0b1;
}

#if 0
	char str[3 + MATRIX_COL];
	str[0] = in + '0';
	str[1] = ':';
	int i;
	for (i = 0; i < MATRIX_COL; i++) {
		if (mat[i])
			str[i + 2] = 'u';
		else
			str[i + 2] = 'd';
	}
	str[MATRIX_COL + 2] = '\n';
	usb_send_serial_data(str, 3 + MATRIX_COL);
#endif

uint8_t kbd_state[MATRIX_ROW][MATRIX_COL];
uint8_t capslock = 0;

void keyboard_parse()
{
    int r, i, j;
    uint8_t state[MATRIX_ROW][MATRIX_COL];

    for (r = 0; r < MATRIX_ROW; r++)
    {
        delay(1); // wait a little between each scan
        gpio_set(KBD_R_PORT, kbd_row_gpio[r]);
        kbd_input_scan(state[r]);
        gpio_clear(KBD_R_PORT, kbd_row_gpio[r]);
    }

    for (i = 0; i < MATRIX_ROW; i++)
    {
        for (j = 0; j < MATRIX_COL; j++)
        {
            if (state[i][j] != kbd_state[i][j])
            {
                uint8_t key = kbd_matrix[i][j];
                if (state[i][j])
                {
                    usb_keyboard_key_down(key);

                    if (key == KEY_CAPS_LOCK)
                    {
                        if (!capslock)
                        {
                            dotsmatrix_led_enable(i, j);
//                            dotsmatrix_led_set_pwm(i, j, 0x40);
                            capslock = 1;
                        }
                        else
                        {
                            dotsmatrix_led_disable(i, j);
                            capslock = 0;
                        }
                    }
                    else
                    {
                        dotsmatrix_led_enable(i, j);
//                        dotsmatrix_led_set_pwm(i, j, 0x40);
                    }
                }
                else
                {
                    usb_keyboard_key_up(key);

                    if (key == KEY_CAPS_LOCK)
                    {
                    }
                    else
                    {
                        dotsmatrix_led_disable(i, j);
                    }
                }
            }
        }
    }
    //    memcpy(cmatrix, matrix, MATRIX_ROW * MATRIX_COL);
    for (i = 0; i < MATRIX_ROW; i++)
        for (j = 0; j < MATRIX_COL; j++)
            kbd_state[i][j] = state[i][j];
}

#define __WFI() __asm__("wfi")
#define __WFE() __asm__("wfe")

void rcc_init()
{
    //#ifdef CUSTOM_BOARD
    // crystal-less ?
    rcc_clock_setup_in_hsi48_out_48mhz();
    rcc_periph_clock_enable(RCC_SYSCFG_COMP);
    SYSCFG_CFGR1 |= SYSCFG_CFGR1_PA11_PA12_RMP;
    crs_autotrim_usb_enable();
    rcc_set_usbclk_source(RCC_HSI48);
    //#else
    //	rcc_clock_setup_in_hsi_out_48mhz();
    //#endif

    rcc_periph_clock_enable(RCC_GPIOA);
    rcc_periph_clock_enable(RCC_GPIOB);
    rcc_periph_clock_enable(RCC_GPIOF);

    rcc_periph_clock_enable(RCC_I2C1);
}

void sys_init()
{
    rcc_init();
    gpio_setup();
    systick_setup();
    dotsmatrix_setup();
    usb_setup();
}

void pow_init()
{
    SCB_SCR &= ~SCB_SCR_SLEEPDEEP;
    SCB_SCR |= SCB_SCR_SLEEPONEXIT;
}

#include <libopencm3/stm32/i2c.h>

void dump_uint8(uint8_t v)
{
    int i;
    char str[9];

    for (i = 0; i < 8; i++)
    {
        if ((v >> i) & 0b1)
            str[7 - i] = '1';
        else
            str[7 - i] = '0';
    }

    str[8] = '\n';
    usb_send_serial_data(str, 9);
}

void dump_uint16(uint16_t v)
{
    int i;
    char str[17];
    for (i = 0; i < 16; i++)
    {
        if ((v >> i) & 0b1)
            str[15 - i] = '1';
        else
            str[15 - i] = '0';
    }
    str[16] = '\n';
    usb_send_serial_data(str, 17);
}

void test_dotsmatrix()
{
    int i, j;

    for (i = 0; i < 5; i++)
    {
        for (j = 0; j < 14; j++)
        {
//            dotsmatrix_led_enable(i, j);
//            dotsmatrix_led_set_pwm(i, j, 0x20);
            			dotsmatrix_led_set_breath(i, j, 0x01);
        }
    }
}

int main(void)
{
    //    memset(cmatrix,0,MATRIX_ROW*MATRIX_COL);
    sys_init();
    delay(100);

    test_dotsmatrix();

//    delay(100);
#ifdef USB_USE_INT
//	pow_init();
#endif

    while (1)
    {
        keyboard_parse();
        delay(10);
        usb_send_keys_if_changed();

#ifdef USB_USE_INT
//		__WFI();
#else
        usb_poll();
#endif
    }

    return 0;
}
