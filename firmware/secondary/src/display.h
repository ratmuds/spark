#ifndef DISPLAY_H
#define DISPLAY_H

#include "lvgl.h"
#include <stdint.h>

// Initialize display hardware, SPI, and register LVGL display drivers
void display_init(void);

void spi_master_init(void);
uint8_t spi_master_transceive(uint8_t command);

#endif // DISPLAY_H