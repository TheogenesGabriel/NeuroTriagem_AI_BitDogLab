#include "ssd1306.h"
#include <string.h>
#include "pico/stdlib.h"

typedef struct { char c; uint8_t col[5]; } glyph_t;

static const glyph_t FONT[] = {
    {' ', {0x00,0x00,0x00,0x00,0x00}},
    {'A', {0x7E,0x11,0x11,0x11,0x7E}},
    {'B', {0x7F,0x49,0x49,0x49,0x36}},
    {'C', {0x3E,0x41,0x41,0x41,0x22}},
    {'D', {0x7F,0x41,0x41,0x41,0x3E}},
    {'E', {0x7F,0x49,0x49,0x49,0x41}},
    {'F', {0x7F,0x09,0x09,0x09,0x01}},
    {'G', {0x3E,0x41,0x49,0x49,0x7A}},
    {'H', {0x7F,0x08,0x08,0x08,0x7F}},
    {'I', {0x00,0x41,0x7F,0x41,0x00}},
    {'J', {0x20,0x40,0x41,0x3F,0x01}},
    {'K', {0x7F,0x08,0x14,0x22,0x41}},
    {'L', {0x7F,0x40,0x40,0x40,0x40}},
    {'M', {0x7F,0x02,0x0C,0x02,0x7F}},
    {'N', {0x7F,0x04,0x08,0x10,0x7F}},
    {'O', {0x3E,0x41,0x41,0x41,0x3E}},
    {'P', {0x7F,0x09,0x09,0x09,0x06}},
    {'Q', {0x3E,0x41,0x51,0x21,0x5E}},
    {'R', {0x7F,0x09,0x19,0x29,0x46}},
    {'S', {0x46,0x49,0x49,0x49,0x31}},
    {'T', {0x01,0x01,0x7F,0x01,0x01}},
    {'U', {0x3F,0x40,0x40,0x40,0x3F}},
    {'V', {0x1F,0x20,0x40,0x20,0x1F}},
    {'W', {0x3F,0x40,0x38,0x40,0x3F}},
    {'X', {0x63,0x14,0x08,0x14,0x63}},
    {'Y', {0x07,0x08,0x70,0x08,0x07}},
    {'Z', {0x61,0x51,0x49,0x45,0x43}},
    {':', {0x00,0x36,0x36,0x00,0x00}},
    {')', {0x00,0x41,0x36,0x08,0x00}},
    {'.', {0x00,0x60,0x60,0x00,0x00}},
    {'0', {0x3E,0x51,0x49,0x45,0x3E}},
    {'1', {0x00,0x42,0x7F,0x40,0x00}},
    {'2', {0x42,0x61,0x51,0x49,0x46}},
    {'3', {0x21,0x41,0x45,0x4B,0x31}},
    {'4', {0x18,0x14,0x12,0x7F,0x10}},
    {'5', {0x27,0x45,0x45,0x45,0x39}},
    {'6', {0x3C,0x4A,0x49,0x49,0x30}},
    {'7', {0x01,0x71,0x09,0x05,0x03}},
    {'8', {0x36,0x49,0x49,0x49,0x36}},
    {'9', {0x06,0x49,0x49,0x29,0x1E}},
};
#define FONT_COUNT (sizeof(FONT) / sizeof(FONT[0]))

static const uint8_t *font_lookup(char c) {
    if (c >= 'a' && c <= 'z') c -= 32; // fold lowercase to uppercase
    for (size_t i = 0; i < FONT_COUNT; i++) {
        if (FONT[i].c == c) return FONT[i].col;
    }
    return FONT[0].col; // unknown char -> blank
}

// --- Low level I2C helpers ----------------------------------------------
static void ssd1306_cmd(uint8_t cmd) {
    uint8_t buf[2] = {0x00, cmd}; // 0x00 = control byte, "command follows"
    i2c_write_blocking(OLED_I2C_PORT, OLED_I2C_ADDR, buf, 2, false);
}

void ssd1306_init(ssd1306_t *disp) {
    i2c_init(OLED_I2C_PORT, 400 * 1000);
    gpio_set_function(OLED_SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(OLED_SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(OLED_SDA_PIN);
    gpio_pull_up(OLED_SCL_PIN);

    static const uint8_t init_cmds[] = {
        0xAE,             // display off
        0x20, 0x00,       // memory addressing mode: horizontal
        0xB0,             // page start address 0
        0xC8,             // COM scan direction: remapped
        0x00, 0x10,       // lower/upper column start address
        0x40,             // display start line 0
        0x81, 0x7F,       // contrast
        0xA1,             // segment re-map
        0xA6,             // normal (not inverted) display
        0xA8, 0x3F,       // multiplex ratio = 63 (128x64 panel)
        0xA4,             // resume to RAM content display
        0xD3, 0x00,       // display offset = 0
        0xD5, 0x80,       // display clock divide ratio / osc freq
        0xD9, 0xF1,       // pre-charge period
        0xDA, 0x12,       // COM pins hardware configuration
        0xDB, 0x40,       // VCOMH deselect level
        0x8D, 0x14,       // charge pump enable
        0xAF,             // display on
    };
    for (size_t i = 0; i < sizeof(init_cmds); i++) {
        ssd1306_cmd(init_cmds[i]);
    }

    ssd1306_clear(disp);
    ssd1306_show(disp);
}

void ssd1306_clear(ssd1306_t *disp) {
    memset(disp->buffer, 0, sizeof(disp->buffer));
}

static inline void set_pixel(ssd1306_t *disp, int x, int y) {
    if (x < 0 || x >= OLED_WIDTH || y < 0 || y >= OLED_HEIGHT) return;
    int page = y / 8;
    disp->buffer[page * OLED_WIDTH + x] |= (1u << (y % 8));
}

void ssd1306_draw_pixel(ssd1306_t *disp, int x, int y) {
    set_pixel(disp, x, y);
}

void ssd1306_draw_bitmap(ssd1306_t *disp, int x, int y, const uint8_t *bitmap, int width, int height) {
    int bytes_per_row = (width + 7) / 8;
    for (int row = 0; row < height; row++) {
        for (int col = 0; col < width; col++) {
            int byte_i = col / 8;
            int bit_i = 7 - (col % 8); // MSB-first within each byte
            uint8_t byte_val = bitmap[row * bytes_per_row + byte_i];
            if (byte_val & (1u << bit_i)) {
                set_pixel(disp, x + col, y + row);
            }
        }
    }
}

void ssd1306_draw_char(ssd1306_t *disp, int x, int y, char c, uint8_t scale) {
    if (scale < 1) scale = 1;
    const uint8_t *glyph = font_lookup(c);
    for (int col = 0; col < 5; col++) {
        uint8_t bits = glyph[col];
        for (int row = 0; row < 7; row++) {
            if (bits & (1u << row)) {
                for (int sx = 0; sx < scale; sx++) {
                    for (int sy = 0; sy < scale; sy++) {
                        set_pixel(disp, x + col * scale + sx, y + row * scale + sy);
                    }
                }
            }
        }
    }
}

void ssd1306_draw_string(ssd1306_t *disp, int x, int y, const char *str, uint8_t scale) {
    if (scale < 1) scale = 1;
    int cursor_x = x;
    int char_width = 6 * scale; // 5 px glyph + 1 px spacing, scaled
    while (*str) {
        ssd1306_draw_char(disp, cursor_x, y, *str, scale);
        cursor_x += char_width;
        str++;
    }
}

void ssd1306_draw_rect(ssd1306_t *disp, int x, int y, int w, int h) {
    for (int i = 0; i < w; i++) {
        set_pixel(disp, x + i, y);
        set_pixel(disp, x + i, y + h - 1);
    }
    for (int i = 0; i < h; i++) {
        set_pixel(disp, x, y + i);
        set_pixel(disp, x + w - 1, y + i);
    }
}


void ssd1306_show(ssd1306_t *disp) {
    for (int page = 0; page < OLED_PAGES; page++) {
        ssd1306_cmd(0xB0 + page); // set page address
        ssd1306_cmd(0x00);        // lower column = 0
        ssd1306_cmd(0x10);        // upper column = 0

        uint8_t chunk[OLED_WIDTH + 1];
        chunk[0] = 0x40; // control byte: data stream follows
        memcpy(&chunk[1], &disp->buffer[page * OLED_WIDTH], OLED_WIDTH);
        i2c_write_blocking(OLED_I2C_PORT, OLED_I2C_ADDR, chunk, OLED_WIDTH + 1, false);
    }
}