#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "lvgl.h"
#include "display.h"

// Global UI elements
static lv_obj_t * capture_count_label = NULL;

// Capture button callback
static void capture_btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    
    if(code == LV_EVENT_CLICKED) {
        printf("Capture button clicked!\n");
        
        // Send capture command to main MCU via SPI
        uint8_t response = spi_master_transceive(0x01); // CMD_CAPTURE = 0x01
        
        // Update the label with the response
        if (capture_count_label != NULL) {
            char buffer[32];
            snprintf(buffer, sizeof(buffer), "Count: %u", response);
            lv_label_set_text(capture_count_label, buffer);
            printf("Received capture count: %u\n", response);
        }
    }
}

// Power supply callback
static void btn_event_cb(lv_event_t * e) {
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t * btn = lv_event_get_current_target(e);
    lv_obj_t * label = lv_obj_get_child(btn, 0);

    if(code == LV_EVENT_VALUE_CHANGED) {
        if(lv_obj_has_state(btn, LV_STATE_CHECKED)) {
            lv_label_set_text(label, "PSU: ON");
            // Add hardware logic to enable PSU here
            printf("Power Supply Enabled\n");
        }
        else {
            lv_label_set_text(label, "PSU: OFF");
            // Add hardware logic to disable PSU here
            printf("Power Supply Disabled\n");
        }
    }
}

void create_gui(void) {
    // Top-level display configurations
    static lv_style_t main_style;
    lv_style_init(&main_style);
    lv_style_set_bg_color(&main_style, lv_color_hex(0x000000));
    lv_style_set_text_color(&main_style, lv_color_hex(0xFFFFFF));
    
    lv_obj_add_style(lv_screen_active(), &main_style, 0);

    // Spark Title
    lv_obj_t * label = lv_label_create(lv_screen_active());
    lv_label_set_text(label, "Spark");
    lv_obj_set_style_text_font(label, &lv_font_montserrat_14, 0); // Need to enable font in lv_conf.h!
    lv_obj_align(label, LV_ALIGN_TOP_MID, 0, 5);

    // Power Toggle Button
    lv_obj_t * btn = lv_btn_create(lv_screen_active());
    lv_obj_add_flag(btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_size(btn, 100, 40);
    lv_obj_align(btn, LV_ALIGN_TOP_MID, 0, 25);
    lv_obj_add_event_cb(btn, btn_event_cb, LV_EVENT_ALL, NULL);

    lv_obj_t * btn_label = lv_label_create(btn);
    lv_label_set_text(btn_label, "PSU: OFF");
    lv_obj_center(btn_label);

    // Capture Button
    lv_obj_t * capture_btn = lv_btn_create(lv_screen_active());
    lv_obj_set_size(capture_btn, 120, 40);
    lv_obj_align(capture_btn, LV_ALIGN_CENTER, 0, 5);
    lv_obj_add_event_cb(capture_btn, capture_btn_event_cb, LV_EVENT_CLICKED, NULL);

    lv_obj_t * capture_btn_label = lv_label_create(capture_btn);
    lv_label_set_text(capture_btn_label, "Capture");
    lv_obj_center(capture_btn_label);

    // Capture Count Label
    capture_count_label = lv_label_create(lv_screen_active());
    lv_label_set_text(capture_count_label, "Count: --");
    lv_obj_align(capture_count_label, LV_ALIGN_CENTER, 0, 50);
}

int main()
{
    stdio_init_all();
    sleep_ms(2000);

    // Initialize LVGL core
    lv_init();

    // Initialize SPI, ST7735 and LVGL display driver
    display_init();

    // Create the UI with elements
    create_gui();

    // Main event loop
    uint32_t last_tick = to_ms_since_boot(get_absolute_time());

    while (true) {
        uint32_t current_tick = to_ms_since_boot(get_absolute_time());
        uint32_t diff = current_tick - last_tick;
        lv_tick_inc(diff);
        last_tick = current_tick;

        lv_timer_handler();
        sleep_ms(5);
    }
}
