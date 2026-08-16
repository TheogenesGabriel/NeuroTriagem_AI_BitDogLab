#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include <stdbool.h>
#include "hardware/i2c.h"

// BitDogLab wires the OLED to I2C1 by default: SDA=GPIO14, SCL=GPIO15.
// Adjust these three defines if your board revision differs.
#define OLED_I2C_PORT   i2c1
#define OLED_SDA_PIN    14
#define OLED_SCL_PIN    15
#define OLED_I2C_ADDR   0x3C

#define OLED_WIDTH      128
#define OLED_HEIGHT     64
#define OLED_PAGES      (OLED_HEIGHT / 8)

typedef struct {
    uint8_t buffer[OLED_WIDTH * OLED_PAGES];
} ssd1306_t;

void ssd1306_init(ssd1306_t *disp);
void ssd1306_clear(ssd1306_t *disp);

void ssd1306_draw_char(ssd1306_t *disp, int x, int y, char c, uint8_t scale);
void ssd1306_draw_string(ssd1306_t *disp, int x, int y, const char *str, uint8_t scale);

void ssd1306_draw_pixel(ssd1306_t *disp, int x, int y);
void ssd1306_draw_rect(ssd1306_t *disp, int x, int y, int w, int h);
void ssd1306_draw_bitmap(ssd1306_t *disp, int x, int y,const uint8_t *bitmap, int width, int height);

void ssd1306_show(ssd1306_t *disp);

#endif // SSD1306_H