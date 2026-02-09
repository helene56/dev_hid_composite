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

enum row
{
    ROW1 = 4,
    ROW2 = 6,
    ROW3 = 9,
};

static uint32_t blink_interval_ms = BLINK_NOT_MOUNTED;
static enum col col_arr[] = {COLA, COLB, COLC};

static volatile uint8_t row_mask = 0;
static volatile uint8_t col_state[3] = {0, 0, 0};

uint8_t mapped_keys[3][3] = {HID_KEY_0, HID_KEY_1, HID_KEY_2,
                             HID_KEY_3, HID_KEY_4, HID_KEY_5,
                             HID_KEY_C, HID_KEY_7, HID_KEY_V};

int current_col_idx = 0;

void led_blinking_task(void);
void hid_task(void);
void scan_btn_matrix(void);
void custom_cdc_task(void);

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
    // let pico sdk use the first cdc interface for std io
    stdio_init_all();

    // rows
    gpio_init(ROW1);
    gpio_pull_up(ROW1);
    gpio_set_dir(ROW1, false);
    gpio_init(ROW2);
    gpio_pull_up(ROW2);
    gpio_set_dir(ROW2, false);
    gpio_init(ROW3);
    gpio_pull_up(ROW3);
    gpio_set_dir(ROW3, false);

    // cols
    gpio_init(COLA);
    gpio_set_dir(COLA, true);
    gpio_put(COLA, true);
    gpio_init(COLB);
    gpio_set_dir(COLB, true);
    gpio_put(COLB, true);
    gpio_init(COLC);
    gpio_set_dir(COLC, true);
    gpio_put(COLC, true);

    while (1)
    {
        tud_task(); // tinyusb device task
        led_blinking_task();

        scan_btn_matrix();
        hid_task();
        // custom_cdc_task();
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
    int modifier = 0;

    if (btn)
    {
        uint8_t keycode[6] = {0};

        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
            {
                bool rc = (col_state[c] & (1u << r)) != 0;
                
                if (rc)
                {
                    if (r == 2 && c == 0)
                    {
                        modifier = KEYBOARD_MODIFIER_LEFTCTRL;
                    }
                    else if (r == 2 && c == 2)
                    {
                        modifier = KEYBOARD_MODIFIER_LEFTCTRL;
                    }
                    keycode[0] = mapped_keys[r][c];
                }
                    
            }
        }

        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
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

    // uint32_t const btn = board_button_read();abababababababababab

    // bool any = btnA_pressed || btnB_pressed;
    uint8_t matrix_mask = col_state[0] | col_state[1] | col_state[2];
    bool any = (matrix_mask & 0b0111) != 0;
    uint32_t const btn = any;
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

    int active_col = current_col_idx;

    // set all columns high (deactivate)
    gpio_put(COLA, true);
    gpio_put(COLB, true);
    gpio_put(COLC, true);
    // set current col to active
    gpio_put(col_arr[active_col], false);
    sleep_us(2);
    // read rows
    row_mask = ((1 ? !gpio_get(ROW3) : 0) << 2) | ((1 ? !gpio_get(ROW2) : 0) << 1) | ((1 ? !gpio_get(ROW1) : 0) << 0);
    // update states
    col_state[active_col] = row_mask;

    // advance column
    current_col_idx++;
    if (current_col_idx > 2)
        current_col_idx = 0;
}



void custom_cdc_task(void)
{
    // polling CDC interfaces if wanted

    // Check if CDC interface 0 (for pico sdk stdio) is connected and ready

    if (tud_cdc_n_connected(0)) {
        // print on CDC 0 some debug message
        printf("Connected to CDC 0\n");
        sleep_ms(5); // wait for 5 seconds
    }
}

// callback when data is received on a CDC interface
void tud_cdc_rx_cb(uint8_t itf)
{
    // allocate buffer for the data in the stack
    uint8_t buf[CFG_TUD_CDC_RX_BUFSIZE];

    printf("RX CDC %d\n", itf);

    // read the available data 
    // | IMPORTANT: also do this for CDC0 because otherwise
    // | you won't be able to print anymore to CDC0
    // | next time this function is called
    uint32_t count = tud_cdc_n_read(itf, buf, sizeof(buf));

    // check if the data was received on the second cdc interface
    if (itf == 1) {
        // process the received data
        if (buf[0] == 0x4f) // "O"
        {
            blink_interval_ms = 0;
            board_led_write(true);
            
        }
        else if (buf[0] == 0x41) // A
        {
            mapped_keys[0][0] = HID_KEY_A;
        }
        else if (buf[0] = 0x30) // 0
        {
            mapped_keys[0][0] = HID_KEY_0;
        }
        buf[count] = 0; // null-terminate the string
        // now echo data back to the console on CDC 0
        printf("Received on CDC 1: %s\n", buf);

        // and echo back OK on CDC 1
        tud_cdc_n_write(itf, (uint8_t const *) "OK\r\n", 4);
        tud_cdc_n_write_flush(itf);
    }
}