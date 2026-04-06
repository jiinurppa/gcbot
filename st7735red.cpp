#include "BMSPA_font.h"
#include "st7735red.hpp"
#include <pico/stdlib.h>
#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <algorithm>
#include <cstdint>
#include <string>

// Slightly modified generic init sequence from:
// https://github.com/bablokb/pico-st7735/blob/main/lib-st7735/src/ST7735_TFT.c
void ST7735Red::write_display_settings() const
{
    write_command(ST7735_SWRESET);
    busy_wait_ms(150);
    write_command(ST7735_SLPOUT);
    busy_wait_ms(500);
    write_command(ST7735_FRMCTR1);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_command(ST7735_FRMCTR2);
    write_data(0x01);
    write_data(0x2C);
    write_data(0x2D);
    write_command(ST7735_FRMCTR3);
    write_data(0x01); write_data(0x2C); write_data(0x2D);
    write_data(0x01); write_data(0x2C); write_data(0x2D);
    write_command(ST7735_INVCTR);
    write_data(0x07);
    write_command(ST7735_PWCTR1);
    write_data(0xA2);
    write_data(0x02);
    write_data(0x84);
    write_command(ST7735_PWCTR2);
    write_data(0xC5);
    write_command(ST7735_PWCTR3);
    write_data(0x0A);
    write_data(0x00);
    write_command(ST7735_PWCTR4);
    write_data(0x8A);
    write_data(0x2A);
    write_command(ST7735_PWCTR5);
    write_data(0x8A);
    write_data(0xEE);
    write_command(ST7735_VMCTR1);
    write_data(0x0E);
    write_command(ST7735_INVOFF);
    write_command(ST7735_MADCTL);
    write_data(0x60);
    write_command(ST7735_COLMOD);
    write_data(0x05);

    write_command(ST7735_RASET);
    write_data(0x00); write_data(0x02);
    write_data(0x00); write_data(0x81);
    write_command(ST7735_CASET);
    write_data(0x00); write_data(0x01);
    write_data(0x00); write_data(0xA0);

    write_command(ST7735_GMCTRP1);
    write_data(0x02); write_data(0x1C); write_data(0x07); write_data(0x12);
    write_data(0x37); write_data(0x32); write_data(0x29); write_data(0x2D);
    write_data(0x29); write_data(0x25); write_data(0x2B); write_data(0x39);
    write_data(0x00); write_data(0x01); write_data(0x03); write_data(0x10);
    write_command(ST7735_GMCTRN1);
    write_data(0x03); write_data(0x1D); write_data(0x07); write_data(0x06);
    write_data(0x2E); write_data(0x2C); write_data(0x29); write_data(0x2D);
    write_data(0x2E); write_data(0x2E); write_data(0x37); write_data(0x3F);
    write_data(0x00); write_data(0x00); write_data(0x02); write_data(0x10);
    write_command(ST7735_NORON);
    busy_wait_ms(10);
    write_command(ST7735_DISPON);
    busy_wait_ms(100);
}

void ST7735Red::init() const
{
    reset();
    gpio_put(_a0, false);
    write_display_settings();
}

ST7735Red::ST7735Red(spi_inst_t* spi, uint cs, uint sck, uint sda, uint a0, uint reset, uint backlight)
{
    _spi = spi;
    spi_init(spi, SPI_BADURATE);
    // SPI bus clock signal
    gpio_set_function(sck, GPIO_FUNC_SPI);
    // SPI bus write data signal (MOSI)
    gpio_set_function(sda, GPIO_FUNC_SPI);

    // LCD chip select signal, low level enable
    _cs = cs;
    gpio_init(cs);
    gpio_set_dir(cs, GPIO_OUT);
    gpio_put(cs, true); 

    // LCD register / data selection signal (DC),
    // high level: register, low level: data 
    _a0 = a0;
    gpio_init(a0);
    gpio_set_dir(a0, GPIO_OUT);
    gpio_put(a0, false);

    // LCD reset signal, low level reset
    _reset = reset;
    gpio_init(reset);
    gpio_set_dir(reset, GPIO_OUT);
    gpio_put(reset, true);

    // Backlight control (LED), high level lighting
    gpio_init(backlight);
    gpio_set_dir(backlight, true);
    gpio_put(backlight, true);

    init();
}

void ST7735Red::reset() const
{
    gpio_put(_reset, true);
    busy_wait_ms(10);
    gpio_put(_reset, false);
    busy_wait_ms(10);
    gpio_put(_reset, true);
    busy_wait_ms(10);
}

void ST7735Red::set_address_window(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) const
{
    // Column address
    write_command(ST7735_CASET);
    write_data(0);
    write_data(x0);
    write_data(0);
    write_data(x1 - 1);
    // Row address
    write_command(ST7735_RASET);
    write_data(0);
    write_data(y0);
    write_data(0);
    write_data(y1 - 1);
    write_command(ST7735_RAMWR);
}

void ST7735Red::write_buffer_to_display(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) const
{
    set_address_window(x0, y0, x1, y1);
    spi_set_format(_spi, 16, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
    gpio_put(_a0, true);
    gpio_put(_cs, false);
    size_t len = x1 - x0;

    for (uint8_t y = y0; y < y1; ++y)
    {
        spi_write16_blocking(_spi, &_display_buffer[y][x0], len);
    }

    gpio_put(_cs, true);
    spi_set_format(_spi, 8, SPI_CPOL_0, SPI_CPHA_0, SPI_MSB_FIRST);
}

void ST7735Red::clear_buffer(uint16_t color)
{
   for (uint8_t y = 0; y < TFT_HEIGHT; ++y)
    {
        std::fill(_display_buffer[y].begin(), _display_buffer[y].end(), color);
    }
}

void ST7735Red::invert_colors(bool inverted) const
{
    write_command(inverted ? ST7735_INVON : ST7735_INVOFF);
}

void ST7735Red::draw_pixel(uint8_t x, uint8_t y, uint16_t color)
{
    _display_buffer[y][x] = color;
}

void ST7735Red::draw_horizontal_line(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color)
{
    for (uint8_t y0 = y; y0 < y + height; ++y0)
    {
        auto x0 = _display_buffer[y0].begin() + x;
        std::fill(x0, x0 + width, color);
    }
}

void ST7735Red::draw_character(char character, uint8_t x, uint8_t y, uint16_t color)
{
    if (character <= ' ' || character >= 0x7F)
    {
        return;
    }

    uint8_t c = character - ' ';

    for (uint8_t j = 0; j < FONT_WIDTH; ++j)
    {
        if (x + j >= TFT_WIDTH)
        {
            return;
        }

        for (uint8_t i = 0; i < FONT_HEIGHT; ++i)
        {
            if ((font[c][j] >> i) & 1)
            {
                if (y + i >= TFT_HEIGHT)
                {
                    break;
                }

                draw_pixel(x + j, y + i, color);
            }
        }
    }
}

void ST7735Red::draw_text(const std::string& text, uint8_t x, uint8_t y, uint16_t color)
{
    uint8_t x_pos = 0;

    for (auto c : text)
    {
        draw_character(c, x + x_pos, y, color);
        x_pos += FONT_WIDTH;
    }
}
