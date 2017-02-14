#ifndef USB_H
#define USB_H
#include <stdlib.h>

#include <libopencm3/stm32/st_usbfs.h>

#include <libopencm3/usb/usbd.h>
#include <libopencm3/usb/cdc.h>

#define USB_USE_INT 1

#define USB_PORT GPIOA
#define USB_DM_PIN GPIO11
#define USB_DP_PIN GPIO12

void usb_setup(void);


void usb_keyboard_keys_up(void);
void usb_keyboard_key_up(uint8_t usb_keycode);
void usb_keyboard_key_down(uint8_t usb_keycode);

uint32_t usb_send_serial_data(void *buf, int len);
uint32_t usb_send_keyboard_report(void);
uint32_t usb_send_keys_if_changed(void);

void usb_poll(void);

#endif