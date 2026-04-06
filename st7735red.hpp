#ifndef ST7735_HPP
#define ST7735_HPP

#include <hardware/gpio.h>
#include <hardware/spi.h>
#include <cstdint>
#include <string>
#include <array>

class ST7735Red
{
private:
    // Commands
    static constexpr uint8_t ST7735_NOP        = 0x00;
    static constexpr uint8_t ST7735_SWRESET    = 0x01;
    static constexpr uint8_t ST7735_RDDID      = 0x04;
    static constexpr uint8_t ST7735_RDDST      = 0x09;
    static constexpr uint8_t ST7735_SLPIN      = 0x10;
    static constexpr uint8_t ST7735_SLPOUT     = 0x11;
    static constexpr uint8_t ST7735_PTLON      = 0x12;
    static constexpr uint8_t ST7735_NORON      = 0x13;
    static constexpr uint8_t ST7735_INVOFF     = 0x20;
    static constexpr uint8_t ST7735_INVON      = 0x21;
    static constexpr uint8_t ST7735_DISPOFF    = 0x28;
    static constexpr uint8_t ST7735_DISPON     = 0x29;
    static constexpr uint8_t ST7735_CASET      = 0x2A;
    static constexpr uint8_t ST7735_RASET      = 0x2B;
    static constexpr uint8_t ST7735_RAMWR      = 0x2C;
    static constexpr uint8_t ST7735_RAMRD      = 0x2E;
    static constexpr uint8_t ST7735_PTLAR      = 0x30;
    static constexpr uint8_t ST7735_VSCRDEF    = 0x33;
    static constexpr uint8_t ST7735_COLMOD     = 0x3A;
    static constexpr uint8_t ST7735_MADCTL     = 0x36;
    static constexpr uint8_t ST7735_MADCTL_MY  = 0x80;
    static constexpr uint8_t ST7735_MADCTL_MX  = 0x40;
    static constexpr uint8_t ST7735_MADCTL_MV  = 0x20;
    static constexpr uint8_t ST7735_MADCTL_ML  = 0x10;
    static constexpr uint8_t ST7735_MADCTL_RGB = 0x00;
    static constexpr uint8_t ST7735_VSCRSADD   = 0x37;
    static constexpr uint8_t ST7735_FRMCTR1    = 0xB1;
    static constexpr uint8_t ST7735_FRMCTR2    = 0xB2;
    static constexpr uint8_t ST7735_FRMCTR3    = 0xB3;
    static constexpr uint8_t ST7735_INVCTR     = 0xB4;
    static constexpr uint8_t ST7735_DISSET5    = 0xB6;
    static constexpr uint8_t ST7735_PWCTR1     = 0xC0;
    static constexpr uint8_t ST7735_PWCTR2     = 0xC1;
    static constexpr uint8_t ST7735_PWCTR3     = 0xC2;
    static constexpr uint8_t ST7735_PWCTR4     = 0xC3;
    static constexpr uint8_t ST7735_PWCTR5     = 0xC4;
    static constexpr uint8_t ST7735_VMCTR1     = 0xC5;
    static constexpr uint8_t ST7735_RDID1      = 0xDA;
    static constexpr uint8_t ST7735_RDID2      = 0xDB;
    static constexpr uint8_t ST7735_RDID3      = 0xDC;
    static constexpr uint8_t ST7735_RDID4      = 0xDD;
    static constexpr uint8_t ST7735_PWCTR6     = 0xFC;
    static constexpr uint8_t ST7735_GMCTRP1    = 0xE0;
    static constexpr uint8_t ST7735_GMCTRN1    = 0xE1;

    static constexpr uint SPI_BADURATE = 13'000'000;
    static constexpr size_t TFT_WIDTH = 160;
    static constexpr size_t TFT_HEIGHT = 128;

    spi_inst_t* _spi;
    uint _cs, _a0, _reset;
    std::array<std::array<uint16_t, TFT_WIDTH>, TFT_HEIGHT> _display_buffer;

    void write_display_settings() const;
    void set_address_window(uint8_t x0 = 0, uint8_t y0 = 0, uint8_t x1 = TFT_WIDTH, uint8_t y1 = TFT_HEIGHT) const;
    inline void write_to_display(uint8_t msg, bool a0_value) const;
    void write_command(uint8_t command) const { write_to_display(command, false); }
    void write_data(uint8_t data) const { write_to_display(data, true); }
    void init() const;

public:
    ST7735Red(spi_inst_t* spi = spi0, uint cs = 17, uint sck = 18, uint sda = 19, uint a0 = 15, uint reset = 14, uint backlight = 26);
    void reset() const;
    void write_buffer_to_display(uint8_t x0 = 0, uint8_t y0 = 0, uint8_t x1 = TFT_WIDTH, uint8_t y1 = TFT_HEIGHT) const;
    inline uint16_t rgb_to_color(uint8_t r, uint8_t g, uint8_t b) const;
    void clear_buffer(uint16_t color = COLOR_BLACK);
    void clear_buffer(uint8_t r, uint8_t g, uint8_t b) { clear_buffer(rgb_to_color(r, g, b)); }
    void invert_colors(bool inverted) const;
    void draw_pixel(uint8_t x, uint8_t y, uint16_t color = COLOR_WHITE);
    void draw_horizontal_line(uint8_t x, uint8_t y, uint8_t width, uint8_t height, uint16_t color = COLOR_WHITE);
    void draw_character(char character, uint8_t x = 0, uint8_t y = 0, uint16_t color = COLOR_WHITE);
    void draw_text(const std::string& test, uint8_t x = 0, uint8_t y = 0, uint16_t color = COLOR_WHITE);
    constexpr size_t width() { return TFT_WIDTH; }
    constexpr size_t height() { return TFT_HEIGHT; }

    static constexpr uint16_t COLOR_BLACK   = 0x0000;
    static constexpr uint16_t COLOR_GRAY    = 0xA534;
    static constexpr uint16_t COLOR_WHITE   = 0xFFFF;
    static constexpr uint16_t COLOR_RED     = 0xF800;
    static constexpr uint16_t COLOR_GREEN   = 0x07E0;
    static constexpr uint16_t COLOR_BLUE    = 0x001F;
    static constexpr uint16_t COLOR_CYAN    = 0x07FF;
    static constexpr uint16_t COLOR_MAGENTA = 0xF81F;
    static constexpr uint16_t COLOR_YELLOW  = 0xFFE0;
    static constexpr uint16_t COLOR_ORANGE  = 0xFC00;
    static constexpr uint16_t COLOR_PURPLE  = 0x781F;
};

void ST7735Red::write_to_display(uint8_t msg, bool a0_value) const
{
    gpio_put(_a0, a0_value);
    gpio_put(_cs, false);
    spi_write_blocking(_spi, &msg, 1);
    gpio_put(_cs, true);
}

uint16_t ST7735Red::rgb_to_color(uint8_t r, uint8_t g, uint8_t b) const
{
    return ((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3);
}

#endif /* ST7735_HPP */
