
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

#define KEY_NC KEY_RESERVED

uint32_t kbd_col_gpio[MATRIX_COL] = {KBD_C1, KBD_C2, KBD_C3, KBD_C4, KBD_C5, KBD_C6, KBD_C7, KBD_C8, KBD_C9, KBD_C10, KBD_C11, KBD_C12, KBD_C13, KBD_C14};
uint32_t kbd_row_gpio[MATRIX_ROW] = {KBD_R1, KBD_R2, KBD_R3, KBD_R4, KBD_R5};

// ISO
uint8_t kbd_matrix[MATRIX_ROW][MATRIX_COL] = {
    {KEY_ESCAPE, KEY_1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_MINUS, KEY_EQUALS, KEY_BACKSPACE},
    {KEY_TAB, KEY_Q, KEY_W, KEY_E, KEY_R, KEY_T, KEY_Y, KEY_U, KEY_I, KEY_O, KEY_P, KEY_LEFT_BRACE, KEY_RIGHT_BRACE, KEY_NC},
    {KEY_CAPS_LOCK, KEY_A, KEY_S, KEY_D, KEY_F, KEY_G, KEY_H, KEY_J, KEY_K, KEY_L, KEY_SEMICOLON, KEY_QUOTE, KEY_NUMBER, KEY_ENTER},
    {KEY_LEFT_SHIFT, KEY_BACKSLASH, KEY_Z, KEY_X, KEY_C, KEY_V, KEY_B, KEY_N, KEY_M, KEY_COMMA, KEY_PERIOD, KEY_SLASH, KEY_NC, KEY_UP},
    {KEY_CONTROL, KEY_FN, KEY_LEFT_ALT, KEY_NC, KEY_NC, KEY_NC, KEY_SPACE, KEY_NC, KEY_NC, KEY_NC, KEY_RIGHT_ALT, KEY_LEFT, KEY_DOWN, KEY_RIGHT},
};

// Fn modifier
uint8_t kbd_fn_matrix[MATRIX_ROW][MATRIX_COL] = {
    {KEY_NC, KEY_F1, KEY_F2, KEY_F3, KEY_F4, KEY_F5, KEY_F6, KEY_F7, KEY_F8, KEY_F9, KEY_NC, KEY_NC, KEY_NC, KEY_NC},
    {KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC},
    {KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC},
    {KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_RIGHT_SHIFT},
    {KEY_NC, KEY_FN, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_NC, KEY_CONTROL},
};

// ..., KEY_RIGHT_SHIFT
// ..., KEY_RIGHT_GUI, KEY_NC, KEY_CONTROL

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
    //  delay(10);
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

#define LED_MODE_SIMPLE_BACKLIGHT 0
#define LED_MODE_BREATH 1
#define LED_MODE_GAME_OF_LIFE 2

uint8_t led_idle_mode = 2;
uint8_t led_mode = 0;
uint8_t led_val = 0;
uint8_t led_prev_val = 0;

uint8_t kbd_state[MATRIX_ROW][MATRIX_COL];
uint8_t capslock = 0;
uint8_t fn = 0;
uint8_t fn_mod = 0;

uint32_t idle_us;

uint8_t delaying = 0;
uint32_t wake_delay = 0;

uint8_t tick_delay(uint32_t d) {

    if (delaying) {
        if (system_millis - wake_delay >= d) {
            delaying = 0;
            return 1;
        }
    } else {
        delaying = 1;
        wake_delay = system_millis;
    }
    return 0;
}

void led_mode_simple_init() {
    int i, j;
    for (i = 0; i < MATRIX_ROW; i++) {
        for (j = 0; j < MATRIX_COL; j++) {
            dotsmatrix_led_enable(i, j);
            dotsmatrix_led_set_breath(i, j, 0x0);
            dotsmatrix_led_set_pwm(i, j, led_val);
        }
    }
}

void led_mode_breath_init() {
    int i, j;
    for (i = 0; i < MATRIX_ROW; i++) {
        for (j = 0; j < MATRIX_COL; j++) {
            dotsmatrix_led_enable(i, j);
            dotsmatrix_led_set_breath(i, j, 0x01);
        }
    }
}

void led_mode_simple_update_all() {
    int i, j;
    if (led_val == 0) {

        for (i = 0; i < MATRIX_ROW; i++) {
            for (j = 0; j < MATRIX_COL; j++) {
                dotsmatrix_led_set_pwm(i, j, 0);
                dotsmatrix_led_disable(i, j);
            }
        }
    } else {
        for (i = 0; i < MATRIX_ROW; i++) {
            for (j = 0; j < MATRIX_COL; j++) {
                if (led_prev_val == 0)
                    dotsmatrix_led_enable(i, j);
                dotsmatrix_led_set_pwm(i, j, led_val);
            }
        }

    }
}

// Game of life

unsigned gol_grid[MATRIX_ROW][MATRIX_COL];
unsigned gol_ngrid[MATRIX_ROW][MATRIX_COL];

void led_mode_game_of_life_update_all() {
    int i, j;
    for (i = 0; i < MATRIX_ROW; i++) {
        for (j = 0; j < MATRIX_COL; j++) {
            if (gol_ngrid[i][j]) {
                if (gol_grid[i][j] == 0) {
                    dotsmatrix_led_enable(i, j);
                    dotsmatrix_led_set_pwm(i, j, 0xFF);
                }
            } else {
                if (gol_grid[i][j] != 0) {
                    dotsmatrix_led_disable(i, j);
                }
            }
        }
    }
}

void led_mode_game_of_life_tick()//void *u)
{
    if (tick_delay(500)) {
        int y1, x1;

        int i, j;
        for (i = 0; i < MATRIX_ROW; i++) {
            for (j = 0; j < MATRIX_COL; j++) {
                int n = 0;
                for (y1 = i - 1; y1 <= i + 1; y1++)
                    for (x1 = j - 1; x1 <= j + 1; x1++)
                        if (gol_grid[(y1 + MATRIX_ROW) % MATRIX_ROW][(x1 + MATRIX_COL) % MATRIX_COL])
                            n++;

                if (gol_grid[i][j]) n--;
                gol_ngrid[i][j] = (n == 3 || (n == 2 && gol_grid[i][j]));
            }
        }

        led_mode_game_of_life_update_all();

        uint8_t game_over = 1;

        for (i = 0; i < MATRIX_ROW; i++) {
            for (j = 0; j < MATRIX_COL; j++) {
                if (gol_grid[i][j] != gol_ngrid[i][j]) {
                    game_over = 0;
                }
            }
        }

        for (i = 0; i < MATRIX_ROW; i++)
            for (j = 0; j < MATRIX_COL; j++)
                gol_grid[i][j] = gol_ngrid[i][j];

//    delay(500);

        if (game_over) {
            led_mode_game_of_life_init();
        }
    }
}

void led_mode_game_of_life_init()
{
    int i, j;

    srand(system_millis);

    for (i = 0; i < MATRIX_ROW; i++)
        for (j = 0; j < MATRIX_COL; j++)
            dotsmatrix_led_disable(i, j);

    for (i = 0; i < MATRIX_ROW; i++) {
        for (j = 0; j < MATRIX_COL; j++) {
            gol_grid[i][j] = rand() < RAND_MAX / 10 ? 1 : 0;
            gol_ngrid[i][j] = gol_grid[i][j];
        }
    }

    led_mode_game_of_life_update_all();
    delay(500);
}

void led_mode_update_all() {
    switch (led_mode) {
    case LED_MODE_SIMPLE_BACKLIGHT:
        led_mode_simple_update_all();
        break;
    default:
        break;
    }
}

void led_mode_init() {
    switch (led_mode) {
    case LED_MODE_SIMPLE_BACKLIGHT:
        led_mode_simple_init();
        break;
    case LED_MODE_BREATH:
        led_mode_breath_init();
        break;
    case LED_MODE_GAME_OF_LIFE:
        led_mode_game_of_life_init();
        break;
    default:
        break;
    }

}

void led_mode_tick() {
    switch (led_mode) {
    case LED_MODE_GAME_OF_LIFE:
        led_mode_game_of_life_tick();
        break;
    default:
        break;
    }
}


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
                uint8_t key_fn = kbd_fn_matrix[i][j];
                if (state[i][j])
                {
                    switch (key)
                    {
                    case KEY_CAPS_LOCK:
                        if (!capslock)
                        {
                            if (led_val == 0) {
                                dotsmatrix_led_enable(i, j);
                                dotsmatrix_led_set_pwm(i, j, 0xFF);
                            }
                            capslock = 1;
                        }
                        else
                        {
                            if (led_val == 0)
                                dotsmatrix_led_disable(i, j);
                            capslock = 0;
                        }
                        break;
                    case KEY_FN:
                        fn = 1;
                        break;
                    default:
//                        dotsmatrix_led_enable(i, j);
//                        dotsmatrix_led_set_pwm(i, j, 0xFF);
                        break;

                    }


                    // LED control
                    if (fn) {
                        switch (key) {
                        case KEY_0:
                            if (led_mode < 2)
                                led_mode++;
                            else
                                led_mode = 0;

                            led_mode_init();
                            key_fn = KEY_FN;

                            break;
                        case KEY_MINUS:
                            if (led_val > 0) {
                                led_prev_val = led_val;
                                led_val -= 0xf;
                                led_mode_update_all();
                            }
                            key_fn = KEY_FN;
                            break;
                        case KEY_EQUALS:
                            if (led_val < 0xFF) {
                                led_prev_val = led_val;
                                led_val += 0xf;
                                led_mode_update_all();
                            }
                            key_fn = KEY_FN;
                            break;
                        case KEY_BACKSPACE:
                            if (led_idle_mode < 2)
                                led_idle_mode++;
                            else
                                led_idle_mode = 0;

                            key_fn = KEY_FN;

                            break;
                        default:
                            break;
                        }
                    }

                    if (fn && key_fn) {
                        fn_mod = key_fn;
                        key = key_fn;
                    }

                    if (key < KEY_FN)
                        usb_keyboard_key_down(key);

                    idle_us = system_millis;
                }
                else
                {
                    switch (key)
                    {
                    case KEY_CAPS_LOCK:
                        break;
                    case KEY_FN:
                        if (fn_mod)
                            usb_keyboard_key_up(fn_mod);

                        fn_mod = 0;
                        fn = 0;
                        break;
                    default:
//                        dotsmatrix_led_disable(i, j);
                        break;
                    }

                    if (fn && key_fn) {
                        key = key_fn;
                    }

                    if (key < KEY_FN)
                        usb_keyboard_key_up(key);
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
    //  rcc_clock_setup_in_hsi_out_48mhz();
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
//            dotsmatrix_led_set_pwm(i, j, 0x7F);
            //          dotsmatrix_led_set_pwm(i, j, 0xFF);
//                      dotsmatrix_led_set_breath(i, j, 0x01);
        }
    }
}

int main(void)
{
    //    memset(cmatrix,0,MATRIX_ROW*MATRIX_COL);
    sys_init();
    delay(100);

//    test_dotsmatrix();

//    delay(100);
#ifdef USB_USE_INT
//  pow_init();
#endif

    idle_us = system_millis;

    while (1)
    {
        keyboard_parse();
        delay(10);
        usb_send_keys_if_changed();

        if (led_idle_mode > 0) {
            if (system_millis - idle_us > 10000) {
                if (led_mode == 0) {
                    led_mode = led_idle_mode;
                    led_mode_init();
                }
            } else {
                if (led_mode > 0) {
                    led_mode = 0;
                    led_mode_init();
                }
            }
        }

        led_mode_tick();

#ifdef USB_USE_INT
//      __WFI();
#else
        usb_poll();
#endif
    }

    return 0;
}
