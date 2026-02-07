/*
 * The MIT License (MIT)
 *
 * Copyright (c) 2019 Ha Thach (tinyusb.org)
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"

#include "bsp/board_api.h"
#include "tusb.h"

#include "usb_descriptors.h"

//--------------------------------------------------------------------+
// MACRO CONSTANT TYPEDEF PROTYPES
//--------------------------------------------------------------------+

/* Blink pattern
 * - 250 ms  : device not mounted
 * - 1000 ms : device mounted
 * - 2500 ms : device is suspended
 */
enum
{
  BLINK_NOT_MOUNTED = 250,
  BLINK_MOUNTED = 1000,
  BLINK_SUSPENDED = 2500,
};

enum col
{
  COLA = 5,
  COLB = 7,
  COLC = 8,
};


static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
static enum col col_arr[] = {COLA, COLB, COLC};
volatile int current_col_idx = 0;
uint8_t key_0 = 1;
uint8_t key_pressed = 0;
void led_blinking_task(void);
void hid_task(void);
void scan_btn_matrix(void);

/*------------- MAIN -------------*/
int main(void)
{

  board_init();

  // init device stack on configured roothub port
  tud_init(BOARD_TUD_RHPORT);

  if (board_init_after_tusb)
  {
    board_init_after_tusb();
  }

  // rows
  gpio_init(4);
  gpio_pull_up(4);
  gpio_set_dir(4, false);
  gpio_init(6);
  gpio_pull_up(6);
  gpio_set_dir(6, false);
  gpio_init(9);
  gpio_pull_up(9);
  gpio_set_dir(9, false);

  // cols
  gpio_init(5);
  gpio_set_dir(5, true);
  gpio_put(5, true);
  gpio_init(7);
  gpio_set_dir(7, true);
  gpio_put(7, true);
  gpio_init(8);
  gpio_set_dir(8, true);
  gpio_put(8, true);

  while (1)
  {
    tud_task(); // tinyusb device task
    led_blinking_task();

    scan_btn_matrix();
    hid_task();
  }
}

//--------------------------------------------------------------------+
// Device callbacks
//--------------------------------------------------------------------+

// Invoked when device is mounted
void tud_mount_cb(void)
{
  blink_interval_ms = BLINK_MOUNTED;
}

// Invoked when device is unmounted
void tud_umount_cb(void)
{
  blink_interval_ms = BLINK_NOT_MOUNTED;
}

// Invoked when usb bus is suspended
// remote_wakeup_en : if host allow us  to perform remote wakeup
// Within 7ms, device must draw an average of current less than 2.5 mA from bus
void tud_suspend_cb(bool remote_wakeup_en)
{
  (void)remote_wakeup_en;
  blink_interval_ms = BLINK_SUSPENDED;
}

// Invoked when usb bus is resumed
void tud_resume_cb(void)
{
  blink_interval_ms = tud_mounted() ? BLINK_MOUNTED : BLINK_NOT_MOUNTED;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

static void send_hid_report(uint8_t report_id, uint32_t btn)
{
  // skip if hid is not ready yet
  if (!tud_hid_ready())
    return;

  // use to avoid send multiple consecutive zero report for keyboard
  static bool has_keyboard_key = false;

  if (btn)
  {
    uint8_t keycode[6] = {0};
    keycode[0] = HID_KEY_A;
    keycode[1] = HID_KEY_B;

    tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, keycode);
    has_keyboard_key = true;
  }
  else
  {
    // send empty key report if previously has key pressed
    if (has_keyboard_key)
      tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
    has_keyboard_key = false;
  }

}

// Every 10ms, we will sent 1 report for each HID profile (keyboard, mouse etc ..)
// tud_hid_report_complete_cb() is used to send the next report after previous one is complete
void hid_task(void)
{
  // Poll every 10ms
  const uint32_t interval_ms = 10;
  static uint32_t start_ms = 0;

  if (board_millis() - start_ms < interval_ms)
    return; // not enough time
  start_ms += interval_ms;

  // uint32_t const btn = board_button_read();
  // uint32_t const btn = (key_pressed == key_0 && !gpio_get(5));
  uint32_t const btn = key_pressed;
  // uint32_t const btn = !gpio_get(4);

  // Remote wakeup
  if (tud_suspended() && btn)
  {
    // Wake up host if we are in suspend mode
    // and REMOTE_WAKEUP feature is enabled by host
    tud_remote_wakeup();
  }
  else
  {
    // Send the 1st of report chain, the rest will be sent by tud_hid_report_complete_cb()
    send_hid_report(REPORT_ID_KEYBOARD, btn);
  }
}

// Invoked when sent REPORT successfully to host
// Application can use this to send the next report
// Note: For composite reports, report[0] is report ID
void tud_hid_report_complete_cb(uint8_t instance, uint8_t const *report, uint16_t len)
{
  (void)instance;
  (void)report;
  (void)len;
}

// Invoked when received GET_REPORT control request
// Application must fill buffer report's content and return its length.
// Return zero will cause the stack to STALL request
uint16_t tud_hid_get_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t *buffer, uint16_t reqlen)
{
  // TODO not Implemented
  (void)instance;
  (void)report_id;
  (void)report_type;
  (void)buffer;
  (void)reqlen;

  return 0;
}

// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
  (void)instance;

  if (report_type == HID_REPORT_TYPE_OUTPUT)
  {
    // Set keyboard LED e.g Capslock, Numlock etc...
    if (report_id == REPORT_ID_KEYBOARD)
    {
      // bufsize should be (at least) 1
      if (bufsize < 1)
        return;

      uint8_t const kbd_leds = buffer[0];

      if (kbd_leds & KEYBOARD_LED_CAPSLOCK)
      {
        // Capslock On: disable blink, turn led on
        blink_interval_ms = 0;
        board_led_write(true);
      }
      else
      {
        // Caplocks Off: back to normal blink
        board_led_write(false);
        blink_interval_ms = BLINK_MOUNTED;
      }
    }
  }
}

//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void)
{
  static uint32_t start_ms = 0;
  static bool led_state = false;

  // blink is disabled
  if (!blink_interval_ms)
    return;

  // Blink every interval ms
  if (board_millis() - start_ms < blink_interval_ms)
    return; // not enough time
  start_ms += blink_interval_ms;

  board_led_write(led_state);
  led_state = 1 - led_state; // toggle
}


void scan_btn_matrix()
{
  // Poll every 1ms
  const uint32_t interval_ms = 1;
  static uint32_t start_ms = 0;
  if (board_millis() - start_ms < interval_ms)
    return; // not enough time
  start_ms += interval_ms;


  // set all columns high
  gpio_put(5, true);
  gpio_put(7, true);
  gpio_put(8, true);

  // set current col to active
  gpio_put(col_arr[current_col_idx], false);
  bool row4 = !gpio_get(4);
  if (current_col_idx == 1)
  {
      key_pressed = !gpio_get(4);   // row is low => pressed
  }

  // wrap to 0 if 2
  if (++current_col_idx > 2) current_col_idx = 0;
  // read rows
  
  // set next col to active
  // gpio_put(col_arr[current_col_idx], false);

  // // read all rows, find the row that is pressed
  // uint8_t row_pressed = ((!gpio_get(4) | !gpio_get(6) | !gpio_get(9))); // 100

  // uint8_t row_mask = (!gpio_get(4) ? 1u << 0 : 0)
  //            | (!gpio_get(6) ? 1u << 1 : 0)
  //            | (!gpio_get(9) ? 1u << 2 : 0);
  


  // key_pressed = row_mask;

}