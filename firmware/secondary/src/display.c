#include "display.h"
#include <stdio.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "hardware/gpio.h"

// Hardware configuration
#define SPI_PORT spi0
#define PIN_MISO 16
#define PIN_SCK  18
#define PIN_MOSI 19

// Display control pins
#define PIN_CS   12
#define PIN_DC   23
#define PIN_RST  24

#define DISP_WIDTH  128
#define DISP_HEIGHT 160

// Send command byte
static void st7735_cmd(uint8_t cmd) {
    gpio_put(PIN_DC, 0); // DC Low for Command
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &cmd, 1);
    gpio_put(PIN_CS, 1);
}

// Send data byte
static void st7735_data(uint8_t data) {
    gpio_put(PIN_DC, 1); // DC High for Data
    gpio_put(PIN_CS, 0);
    spi_write_blocking(SPI_PORT, &data, 1);
    gpio_put(PIN_CS, 1);
}

// Ensure address constraints
static void st7735_set_address_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1) {
    st7735_cmd(0x2A); // CASET
    st7735_data(0x00);
    st7735_data(x0);
    st7735_data(0x00);
    st7735_data(x1);

    st7735_cmd(0x2B); // RASET
    st7735_data(0x00);
    st7735_data(y0);
    st7735_data(0x00);
    st7735_data(y1);

    st7735_cmd(0x2C); // RAMWR
}

static void st7735_init() {
    // Config SPI pins
    gpio_set_function(PIN_SCK, GPIO_FUNC_SPI);
    gpio_set_function(PIN_MOSI, GPIO_FUNC_SPI);
    
    // The ST7735 doesn't send data back typically, but connect MISO if used
    gpio_set_function(PIN_MISO, GPIO_FUNC_SPI);

    // Initialize CS as outputs
    gpio_init(PIN_CS);
    gpio_set_dir(PIN_CS, GPIO_OUT);
    gpio_put(PIN_CS, 1);
    
    // Initialize DC
    gpio_init(PIN_DC);
    gpio_set_dir(PIN_DC, GPIO_OUT);
    gpio_put(PIN_DC, 1);
    
    // Initialize Reset
    gpio_init(PIN_RST);
    gpio_set_dir(PIN_RST, GPIO_OUT);
    
    // Hardware reset
    gpio_put(PIN_RST, 1);
    sleep_ms(50);
    gpio_put(PIN_RST, 0);
    sleep_ms(50);
    gpio_put(PIN_RST, 1);
    sleep_ms(150);

    // Initialization sequence
    st7735_cmd(0x01); // SWRESET
    sleep_ms(150);
    st7735_cmd(0x11); // SLPOUT
    sleep_ms(500);

    st7735_cmd(0x3A); // PIXFMT
    st7735_data(0x05); // 16-bit color
    
    st7735_cmd(0x36); // MADCTL
    st7735_data(0x00); // Standard orientation
    
    st7735_cmd(0x29); // DISPON
    sleep_ms(100);
}

void display_init(void) {
    spi_init(SPI_PORT, 24000000);
    spi_master_init();
    
    st7735_init();
}

void st7735_flush(lv_display_t * disp, const lv_area_t * area, uint8_t * px_map) {
    uint16_t * colors = (uint16_t *)px_map;
    int32_t width = lv_area_get_width(area);
    int32_t height = lv_area_get_height(area);

    st7735_set_address_window(area->x1, area->y1, area->x2, area->y2);

    gpio_put(PIN_DC, 1);
    gpio_put(PIN_CS, 0);

    // Note: ensure your lv_conf.h has LV_COLOR_16_SWAP 1 if colors look off
    spi_write16_blocking(SPI_PORT, colors, width * height);

    gpio_put(PIN_CS, 1);

    lv_display_flush_ready(disp);
}

void display_init(void) {
    // Initialize SPI
    spi_init(SPI_PORT, 24 * 1000 * 1000); // 24 MHz

    // Setup LVGL display
    lv_display_t * disp = lv_display_create(DISP_WIDTH, DISP_HEIGHT);
    lv_display_set_flush_cb(disp, st7735_flush);
    
    // Allow LVGL to manage partial rendering buffers. 
    // Recommended size for an ST7735 is 1/10 to 1/4 screen buffer
    size_t buf_size = DISP_WIDTH * (DISP_HEIGHT / 10) * sizeof(lv_color16_t);
    void * buf1 = malloc(buf_size);
    void * buf2 = malloc(buf_size);
    lv_display_set_buffers(disp, buf1, buf2, buf_size, LV_DISPLAY_RENDER_MODE_PARTIAL);

    // Init hardware explicitly before rendering!
    st7735_init();
}
// SPI Master configuration
#define SPI_MASTER_PORT spi0 // or spi1
#define SPI_MASTER_TX   4
#define SPI_MASTER_CS   5
#define SPI_MASTER_SCK  6
#define SPI_MASTER_RX   7

void spi_master_init(void) {
    spi_init(spi0, 1000000);
    gpio_set_function(SPI_MASTER_TX, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MASTER_SCK, GPIO_FUNC_SPI);
    gpio_set_function(SPI_MASTER_RX, GPIO_FUNC_SPI);
    gpio_init(SPI_MASTER_CS);
    gpio_set_dir(SPI_MASTER_CS, GPIO_OUT);
    gpio_put(SPI_MASTER_CS, 1);
}

uint8_t spi_master_transceive(uint8_t command) {
    uint8_t response = 0;
    gpio_put(SPI_MASTER_CS, 0);
    sleep_us(10);
    spi_write_blocking(spi0, &command, 1);
    sleep_us(50);
    spi_read_blocking(spi0, 0xFF, &response, 1);
    sleep_us(10);
    gpio_put(SPI_MASTER_CS, 1);
    return response;
}
