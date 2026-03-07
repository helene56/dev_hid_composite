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
#include "pico/flash.h"
#include "hardware/flash.h"

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
// mapped_keys can hold 20 characters
#define KEY_LEN 50

typedef struct {
    uint8_t key_modifier[KEY_LEN];
    uint8_t keys[KEY_LEN];
} KeyCell;


KeyCell mapped_keys[3][3] =
{
    {
        {
            { 0 },
            { HID_KEY_H, HID_KEY_E, HID_KEY_L, HID_KEY_L, HID_KEY_O,
              HID_KEY_SPACE, HID_KEY_W, HID_KEY_O, HID_KEY_R, HID_KEY_L,
              HID_KEY_D, HID_KEY_SPACE }
        },
        {
            { 0 },
            { HID_KEY_1, HID_KEY_2, HID_KEY_3 }
        },
        {
            { 0 },
            { HID_KEY_2 }
        }
    },
    {
        {
            { 0 },
            { HID_KEY_3 }
        },
        {
            { 0 },
            { HID_KEY_4 }
        },
        {
            { 0 },
            { HID_KEY_5 }
        }
    },
    {
        {
            { 0 },
            { HID_KEY_C }
        },
        {
            { 0 },
            { HID_KEY_7 }
        },
        {
            { 0 },
            { HID_KEY_V }
        }
    }
};

KeyCell default_mapped_keys[3][3] =
{
    {
        {
            { 0 },
            { HID_KEY_H, HID_KEY_E, HID_KEY_L, HID_KEY_L, HID_KEY_O,
              HID_KEY_SPACE, HID_KEY_W, HID_KEY_O, HID_KEY_R, HID_KEY_L,
              HID_KEY_D, HID_KEY_SPACE }
        },
        {
            { 0 },
            { HID_KEY_1, HID_KEY_2, HID_KEY_3 }
        },
        {
            { 0 },
            { HID_KEY_2 }
        }
    },
    {
        {
            { 0 },
            { HID_KEY_3 }
        },
        {
            { 0 },
            { HID_KEY_4 }
        },
        {
            { 0 },
            { HID_KEY_5 }
        }
    },
    {
        {
            { 0 },
            { HID_KEY_C }
        },
        {
            { 0 },
            { HID_KEY_7 }
        },
        {
            { 0 },
            { HID_KEY_V }
        }
    }
};

uint8_t const conv_table[128][2] =  { HID_ASCII_TO_KEYCODE };

// uint8_t mapped_keys[3][3] = {HID_KEY_0, HID_KEY_1, HID_KEY_2,
//                                   HID_KEY_3, HID_KEY_4, HID_KEY_5,
//                                   HID_KEY_C, HID_KEY_7, HID_KEY_V};

int current_col_idx = 0;

// flash contents
#define FLASH_TARGET_OFFSET (PICO_FLASH_SIZE_BYTES - FLASH_SECTOR_SIZE)

const uint8_t *flash_target_contents = (const uint8_t *)(XIP_BASE + FLASH_TARGET_OFFSET);

#define COUNT_MAGIC 0xA7F3C91E
// typedef struct {
//     uint32_t magic;
//     uint32_t version;
//     uint32_t counter;
// } SavedCount;

// SavedCount count_data = {.counter = 5, .magic = COUNT_MAGIC, .version = 1};


typedef struct {
    uint32_t magic;
    uint32_t version;
    KeyCell keys[3][3];
} SavedKeys;

SavedKeys persistent_keys = {.magic = COUNT_MAGIC, .version = 1};


#define pages_needed (sizeof(persistent_keys) + FLASH_PAGE_SIZE - 1) / FLASH_PAGE_SIZE
uint8_t page_buffer[pages_needed * FLASH_PAGE_SIZE];

void print_buf(const uint8_t *buf, size_t len) 
{
    for (size_t i = 0; i < len; ++i) {
        printf("%02x", buf[i]);
        if (i % 16 == 15)
            printf("\n");
        else
            printf(" ");
    }
}

// This function will be called when it's safe to call flash_range_erase
static void call_flash_range_erase(void *param) 
{
    uint32_t offset = (uint32_t)param;
    flash_range_erase(offset, FLASH_SECTOR_SIZE);
}

// This function will be called when it's safe to call flash_range_program
static void call_flash_range_program(void *param) 
{
    uint32_t offset = ((uintptr_t*)param)[0];
    const uint8_t *data = (const uint8_t *)((uintptr_t*)param)[1];
    flash_range_program(offset, data, pages_needed * FLASH_PAGE_SIZE);
}



void init_default_key_data()
{
    // init to 0
    memcpy(persistent_keys.keys, mapped_keys, sizeof(persistent_keys.keys));
    // sleep_ms(5000);

    // memset(persistent_keys.keys, 0, sizeof(persistent_keys.keys));
    // for (int r = 0; r<3 ;r++)
    // {
    //     for (int c = 0; c<3;c++)
    //     {
    //         for (int i = 0; i<KEY_LEN;i++)
    //         {
    //             // defaults from mapped keys
    //             // if (!mapped_keys[r][c].keys[i])
    //             // {
    //             //     break;
    //             // }
    //             persistent_keys.keys[r][c].keys[i] = mapped_keys[r][c].keys[i];

    //         }
    //     }
    // }
    
}

static void load_mapped_keys_from_persistent(void)
{
    for (int r = 0; r < 3; r++)
    {
        for (int c = 0; c < 3; c++)
        {
            for (int i = 0; i < KEY_LEN; i++)
            {
                mapped_keys[r][c].keys[i] = persistent_keys.keys[r][c].keys[i];
                mapped_keys[r][c].key_modifier[i] = persistent_keys.keys[r][c].key_modifier[i];
            }
        }
    }
}

void init_count_data()
{
    // copy the count_data into page buffer
    memset(page_buffer, 0xFF, pages_needed * FLASH_PAGE_SIZE);
    memcpy(page_buffer, &persistent_keys, sizeof(persistent_keys));
    // erase flash
    int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
    hard_assert(rc == PICO_OK);
    // program flash
    uintptr_t params[] = { FLASH_TARGET_OFFSET, (uintptr_t)page_buffer};
    rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
    hard_assert(rc == PICO_OK);
    // read back as a struct
    printf("Done. Read back target region:\n");
    print_buf(flash_target_contents, pages_needed * FLASH_PAGE_SIZE);
}

void read_flash_count()
{
    const SavedKeys* flash_data = (const SavedKeys*)(flash_target_contents);
    // read flash struct magic
    if (flash_data->magic == COUNT_MAGIC && flash_data->version == 2)
    {
        // valid
        // copy to ram
        persistent_keys = *flash_data; // copies the whole struct from flash into ram variable
        // load_mapped_keys_from_persistent();
        // count_data.counter++;
    }
    else
    {

        persistent_keys.magic = COUNT_MAGIC;
        init_default_key_data();
        persistent_keys.version = 2;
        // load_mapped_keys_from_persistent();
        init_count_data();
        // count_data.counter = 1;

    }
}






void led_blinking_task(void);
void hid_task(void);
void scan_btn_matrix(void);
void custom_cdc_task(void);
// void save_keys(void);
// void init_key_data(void);

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
    KeyCell test = {
        { 0 },
        { HID_KEY_H, HID_KEY_E, HID_KEY_L, HID_KEY_L, HID_KEY_O }
    };



    // read_flash_count();
    // init_count_data();
    // initialize saved keys
    // init_key_data();
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
        // led_blinking_task();

        scan_btn_matrix();
        hid_task();
        
        // print_buf(flash_target_contents, pages_needed * FLASH_PAGE_SIZE);
        printf("default 1: %d\n", mapped_keys[0][0].keys[0]);
        printf("default key 1: %d\n", default_mapped_keys[0][0].keys[0]);
        printf("test.keys[0] = %u\n", test.keys[0]);
        // printf("sizeof(KeyCell): %zu\n", sizeof(KeyCell));
        // printf("sizeof(SavedKeys): %zu\n", sizeof(SavedKeys));
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
    // send button presses on the rising edge, send button release on the falling edge
    // nothing happens inbetween
    static bool last_btn = false;  // edge tracking
    // TODO
    // I need to figure out how to allow for a modifer value
    int modifier = 0;
    static uint32_t press_time = 0;
    static uint32_t interval_time = 400;
    static bool inital_delay_set = false;

    if (btn && !last_btn)
    {
        uint8_t keycode[6] = {0};
        uint8_t keycodestr[KEY_LEN] = {0};
        uint8_t key_modifiers[KEY_LEN] = {0};

        for (int r = 0; r < 3; r++)
        {
            for (int c = 0; c < 3; c++)
            {
                bool rc = (col_state[c] & (1u << r)) != 0;
                
                if (rc)
                {
                    for (int i = 0; i < KEY_LEN-1; i++)
                    {
                        keycodestr[i] = mapped_keys[r][c].keys[i];
                        key_modifiers[i] = mapped_keys[r][c].key_modifier[i];

                    }
                    
                }
                    
            }
        }

        for (int i = 0; i< KEY_LEN-1 && keycodestr[i]; i++)
        {
            keycode[0] = keycodestr[i];
            modifier = key_modifiers[i];
            // maybe the modifer can be combined in the uint8 keycodestr and extracted here?
            printf("keystroke: %d\n", keycode[0]);
            while (!tud_hid_ready()) tud_task();
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, modifier, keycode);
            while (!tud_hid_ready()) tud_task();
            tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL); // release

        }
        press_time = board_millis();

    }
    if (!btn && last_btn) {         // falling edge: ensure release
        tud_hid_keyboard_report(REPORT_ID_KEYBOARD, 0, NULL);
        // reset timer
        press_time = 0;
        interval_time = 400;
        inital_delay_set = false;
    }

    last_btn = btn;
    if (inital_delay_set)
    {
        // repeat every 10 ms
        interval_time = 10;
    }
    if (last_btn && board_millis() - press_time >= interval_time)
    {

        last_btn = !last_btn;
        inital_delay_set = true;
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


void map_assign_keys(uint8_t const *macro_cmd)
{
    macro_cmd++;
    uint8_t key_id = macro_cmd[0];
    printf("key id gathered from macro: %d\n", key_id);
    key_id--;
    // what if macro_cmd is empty?
    const uint8_t *macro_str = &macro_cmd[1];
    uint row =  key_id / 3;
    uint col =  key_id % 3;
    printf("row: %d, col: %d\n", row, col);
    // memset(&persistent_keys.keys[row][col], 0, sizeof(persistent_keys.keys[row][col]));
    // memset(&mapped_keys[row][col], 0, sizeof(mapped_keys[row][col]));
    // copy the macro_str to be used as the key macro into the mapped_key
    for (int i = 0; i < KEY_LEN-1; i++)
    {
        
        uint8_t hid_keycode;
        uint8_t hid_modifier = 0;
        printf("key char: %c\n", *macro_str);
        switch (*macro_str)
        {
        case 1:
            hid_keycode = conv_table['a'][1];
            hid_modifier = KEYBOARD_MODIFIER_LEFTCTRL;
            break;
        case 2:
            hid_keycode = conv_table['b'][1];
            hid_modifier = KEYBOARD_MODIFIER_LEFTCTRL;
            break;
        case 3:
            hid_keycode = conv_table['c'][1];
            hid_modifier = KEYBOARD_MODIFIER_LEFTCTRL;
            break;
        case 22:
            hid_keycode = conv_table['v'][1];
            hid_modifier = KEYBOARD_MODIFIER_LEFTCTRL;
            break;
        case 25:
            hid_keycode = conv_table['y'][1];
            hid_modifier = KEYBOARD_MODIFIER_LEFTCTRL;
            break;
        case 26:
            hid_keycode = conv_table['z'][1];
            hid_modifier = KEYBOARD_MODIFIER_LEFTCTRL;
            break;

        default:
            hid_keycode = conv_table[*macro_str][1];
            if ( conv_table[*macro_str][0] )
            {
                hid_modifier = KEYBOARD_MODIFIER_LEFTSHIFT;
            }
            
            break;
        }

        printf("keycode: %d\n", hid_keycode);
        // update flash
        // persistent_keys.keys[row][col].keys[i] = hid_keycode;
        // persistent_keys.keys[row][col].key_modifier[i] = hid_modifier;

        mapped_keys[row][col].keys[i] = hid_keycode;
        mapped_keys[row][col].key_modifier[i] = hid_modifier;


        if (!*macro_str)
        {
            break;
        }
        // move to the next char
        macro_str++;
    }
    // safeguard set last val to nullbyte terminator
    mapped_keys[row][col].keys[KEY_LEN-1] = 0;
    // persistent_keys.keys[row][col].keys[KEY_LEN-1] = 0;
    // init_count_data();

}


// Invoked when received SET_REPORT control request or
// received data on OUT endpoint ( Report ID = 0, Type = 0 )
void tud_hid_set_report_cb(uint8_t instance, uint8_t report_id, hid_report_type_t report_type, uint8_t const *buffer, uint16_t bufsize)
{
    (void)instance;

    if (report_type == HID_REPORT_TYPE_OUTPUT)
    {
        if (bufsize < 1)
                return;

        uint8_t const cmd = buffer[0];
        printf("command used: %d\n", cmd);
        printf("report id: %d\n", report_id);
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
        else if (report_id == 0) // for generic this is set to 0
        {
            // todo i need to check now that the first byte from the buffer is the correct id so buffer[0] == report_id_cmd
            // bufsize should be (at least) 1
            if (bufsize < 1)
                return;

            uint8_t const key_id = buffer[1];
            uint8_t const app_cmd = buffer[2];
            printf("key id used: %d\n", key_id);
            if (key_id == 0)
            {
                // Capslock On: disable blink, turn led on5
                blink_interval_ms = 0;
                if (app_cmd == 0x02)
                {
                    board_led_write(true);
                }
                else
                {
                    board_led_write(false);
                }
                

            }
            else
            {
                map_assign_keys(buffer);
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
        else if (buf[0] == 0x46) // "F"
        {
            blink_interval_ms = 0;
            board_led_write(false);
            
        }
        // else if (buf[0] == 0x41) // A
        // {
        //     mapped_keys[0][0] = HID_KEY_A;
        // }
        // else if (buf[0] = 0x30) // 0
        // {
        //     mapped_keys[0][0] = HID_KEY_0;
        // }
        buf[count] = 0; // null-terminate the string
        // now echo data back to the console on CDC 0
        printf("Received on CDC 1: %s\n", buf);

        // and echo back OK on CDC 1
        tud_cdc_n_write(itf, (uint8_t const *) "OK\r\n", 4);
        tud_cdc_n_write_flush(itf);
    }
}






// void save_keys()
// {

//     // uint8_t key_saved_data[FLASH_PAGE_SIZE];
//     // here we should probably save the key data.. after the basic_app has written new keys
//     // Note that a whole number of sectors must be erased at a time.
//     printf("\nErasing target region...\n");
//     // Flash is "execute in place" and so will be in use when any code that is stored in flash runs, e.g. an interrupt handler
//     // or code running on a different core.
//     // Calling flash_range_erase or flash_range_program at the same time as flash is running code would cause a crash.
//     // flash_safe_execute disables interrupts and tries to cooperate with the other core to ensure flash is not in use
//     // See the documentation for flash_safe_execute and its assumptions and limitations
//     int rc = flash_safe_execute(call_flash_range_erase, (void*)FLASH_TARGET_OFFSET, UINT32_MAX);
//     hard_assert(rc == PICO_OK);
//     printf("Done. Read back target region:\n");
//     printf("\nProgramming target region...\n");
//     // read the flash sector

//     uintptr_t params[] = { FLASH_TARGET_OFFSET, (uintptr_t)key_saved_data};
//     rc = flash_safe_execute(call_flash_range_program, params, UINT32_MAX);
//     hard_assert(rc == PICO_OK);
//     printf("Done. Read back target region:\n");

//     bool mismatch = false;
//     for (int r = 0; r < 3; r++)
//     {
//         for (int c = 0; c < 3; c++)
//         {
//             for (int i = 0; i < KEY_LEN-1; i++)
//                 if (key_saved_data[r][c].keys[i] != flash_target_contents[r][c].keys[i])
//                     mismatch = true;
//         }
            
//     }
//     if (mismatch)
//         printf("Programming failed!\n");
//     else
//         printf("Programming successful!\n");
// }
