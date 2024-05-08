/*
 * VERSION: R1.0
 * ST7735 BASED SPI LCD DRIVER LIBRARY
 * This library is a mix of two libraries from the below git paths
 * 	1. https://github.com/afiskon/stm32-st7735.git
 * 	2. https://controllerstech.com/st7735-1-8-tft-display-with-stm32/
 * 	I have included some functions from the second library and made a single one
 * 	It works with any rotation of screen and is optimized with usage of variables and functions
 */
#include "st7735.h"

#include "malloc.h"
#include "string.h"
#include <stdlib.h>

#define DELAY 0x80




// based on Adafruit ST7735 library for Arduino
static const uint8_t
  init_cmds1[] = {            // Init for 7735R, part 1 (red or green tab)
    15,                       // 15 commands in list:
    ST7735_SWRESET,   DELAY,  //  1: Software reset, 0 args, w/delay
      150,                    //     150 ms delay
    ST7735_SLPOUT ,   DELAY,  //  2: Out of sleep mode, 0 args, w/delay
      255,                    //     500 ms delay
    ST7735_FRMCTR1, 3      ,  //  3: Frame rate ctrl - normal mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR2, 3      ,  //  4: Frame rate control - idle mode, 3 args:
      0x01, 0x2C, 0x2D,       //     Rate = fosc/(1x2+40) * (LINE+2C+2D)
    ST7735_FRMCTR3, 6      ,  //  5: Frame rate ctrl - partial mode, 6 args:
      0x01, 0x2C, 0x2D,       //     Dot inversion mode
      0x01, 0x2C, 0x2D,       //     Line inversion mode
    ST7735_INVCTR , 1      ,  //  6: Display inversion ctrl, 1 arg, no delay:
      0x07,                   //     No inversion
    ST7735_PWCTR1 , 3      ,  //  7: Power control, 3 args, no delay:
      0xA2,
      0x02,                   //     -4.6V
      0x84,                   //     AUTO mode
    ST7735_PWCTR2 , 1      ,  //  8: Power control, 1 arg, no delay:
      0xC5,                   //     VGH25 = 2.4C VGSEL = -10 VGH = 3 * AVDD
    ST7735_PWCTR3 , 2      ,  //  9: Power control, 2 args, no delay:
      0x0A,                   //     Opamp current small
      0x00,                   //     Boost frequency
    ST7735_PWCTR4 , 2      ,  // 10: Power control, 2 args, no delay:
      0x8A,                   //     BCLK/2, Opamp current small & Medium low
      0x2A,  
    ST7735_PWCTR5 , 2      ,  // 11: Power control, 2 args, no delay:
      0x8A, 0xEE,
    ST7735_VMCTR1 , 1      ,  // 12: Power control, 1 arg, no delay:
      0x0E,
    ST7735_INVOFF , 0      ,  // 13: Don't invert display, no args, no delay
    ST7735_MADCTL , 1      ,  // 14: Memory access control (directions), 1 arg:
      ST7735_LCD_ROTATION,        //     row addr/col addr, bottom to top refresh
    ST7735_COLMOD , 1      ,  // 15: set color mode, 1 arg, no delay:
      0x05 },                 //     16-bit color

#if (defined(ST7735_LCD_IS_128X128) || defined(ST7735_LCD_IS_160X128))
  init_cmds2[] = {            // Init for 7735R, part 2 (1.44" display)
    2,                        //  2 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F,             //     XEND = 127
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x7F },           //     XEND = 127
#endif // ST7735_LCD_IS_128X128

#ifdef ST7735_LCD_IS_160X80
  init_cmds2[] = {            // Init for 7735S, part 2 (160x80 display)
    3,                        //  3 commands in list:
    ST7735_CASET  , 4      ,  //  1: Column addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x4F,             //     XEND = 79
    ST7735_RASET  , 4      ,  //  2: Row addr set, 4 args, no delay:
      0x00, 0x00,             //     XSTART = 0
      0x00, 0x9F ,            //     XEND = 159
    ST7735_INVON, 0 },        //  3: Invert colors
#endif

  init_cmds3[] = {            // Init for 7735R, part 3 (red or green tab)
    4,                        //  4 commands in list:
    ST7735_GMCTRP1, 16      , //  1: Gamma Adjustments (pos. polarity), 16 args, no delay:
      0x02, 0x1c, 0x07, 0x12,
      0x37, 0x32, 0x29, 0x2d,
      0x29, 0x25, 0x2B, 0x39,
      0x00, 0x01, 0x03, 0x10,
    ST7735_GMCTRN1, 16      , //  2: Gamma Adjustments (neg. polarity), 16 args, no delay:
      0x03, 0x1d, 0x07, 0x06,
      0x2E, 0x2C, 0x29, 0x2D,
      0x2E, 0x2E, 0x37, 0x3F,
      0x00, 0x00, 0x02, 0x10,
    ST7735_NORON  ,    DELAY, //  3: Normal display on, no args, w/delay
      10,                     //     10 ms delay
    ST7735_DISPON ,    DELAY, //  4: Main screen turn on, no args w/delay
      100 };                  //     100 ms delay

static void ST7735_Select() {
    HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_RESET);
}

void ST7735_Unselect() {
    HAL_GPIO_WritePin(ST7735_CS_GPIO_Port, ST7735_CS_Pin, GPIO_PIN_SET);
}

static void ST7735_Reset() {
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_RESET);
    HAL_Delay(5);
    HAL_GPIO_WritePin(ST7735_RES_GPIO_Port, ST7735_RES_Pin, GPIO_PIN_SET);
}

static void ST7735_WriteCommand(uint8_t cmd) {
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, &cmd, sizeof(cmd), HAL_MAX_DELAY);
}

static void ST7735_WriteData(uint8_t* buff, size_t buff_size) {
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    HAL_SPI_Transmit(&ST7735_SPI_PORT, buff, buff_size, HAL_MAX_DELAY);
}

static void ST7735_ExecuteCommandList(const uint8_t *addr) {
    uint8_t numCommands, numArgs;
    uint16_t ms;

    numCommands = *addr++;
    while(numCommands--) {
        uint8_t cmd = *addr++;
        ST7735_WriteCommand(cmd);

        numArgs = *addr++;
        // If high bit set, delay follows args
        ms = numArgs & DELAY;
        numArgs &= ~DELAY;
        if(numArgs) {
            ST7735_WriteData((uint8_t*)addr, numArgs);
            addr += numArgs;
        }

        if(ms) {
            ms = *addr++;
            if(ms == 255) ms = 500;
            HAL_Delay(ms);
        }
    }
}

static void ST7735_SetAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    // column address set
    ST7735_WriteCommand(ST7735_CASET);
    uint8_t data[] = { 0x00, x0 + ST7735_XSTART, 0x00, x1 + ST7735_XSTART };
    ST7735_WriteData(data, sizeof(data));

    // row address set
    ST7735_WriteCommand(ST7735_RASET);
    data[1] = y0 + ST7735_YSTART;
    data[3] = y1 + ST7735_YSTART;
    ST7735_WriteData(data, sizeof(data));

    // write to RAM
    ST7735_WriteCommand(ST7735_RAMWR);
}

void ST7735_Init() {
    ST7735_Select();
    ST7735_Reset();
    ST7735_ExecuteCommandList(init_cmds1);
    ST7735_ExecuteCommandList(init_cmds2);
    ST7735_ExecuteCommandList(init_cmds3);
    ST7735_Unselect();
}
/* DONE*/
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color) {
    if((x >= ST7735_LCD_WIDTH) || (y >= ST7735_LCD_HEIGHT))
        return;

    ST7735_Select();

    ST7735_SetAddressWindow(x, y, x+1, y+1);
    uint8_t data[] = { color >> 8, color & 0xFF };
    ST7735_WriteData(data, sizeof(data));

    ST7735_Unselect();
}

static void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color, uint16_t bgcolor) {
    uint32_t i, b, j;

    ST7735_SetAddressWindow(x, y, x+font.width-1, y+font.height-1);

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                uint8_t data[] = { color >> 8, color & 0xFF };
                ST7735_WriteData(data, sizeof(data));
            } else {
                uint8_t data[] = { bgcolor >> 8, bgcolor & 0xFF };
                ST7735_WriteData(data, sizeof(data));
            }
        }
    }
}

/*
Simpler (and probably slower) implementation:

static void ST7735_WriteChar(uint16_t x, uint16_t y, char ch, FontDef font, uint16_t color) {
    uint32_t i, b, j;

    for(i = 0; i < font.height; i++) {
        b = font.data[(ch - 32) * font.height + i];
        for(j = 0; j < font.width; j++) {
            if((b << j) & 0x8000)  {
                ST7735_DrawPixel(x + j, y + i, color);
            } 
        }
    }
}
*/

void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor) {
    ST7735_Select();

    while(*str) {
        if(x + font.width >= ST7735_LCD_WIDTH) {
            x = 0;
            y += font.height;
            if(y + font.height >= ST7735_LCD_HEIGHT) {
                break;
            }

            if(*str == ' ') {
                // skip spaces in the beginning of the new line
                str++;
                continue;
            }
        }

        ST7735_WriteChar(x, y, *str, font, color, bgcolor);
        x += font.width;
        str++;
    }

    ST7735_Unselect();
}

void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= ST7735_LCD_WIDTH) || (y >= ST7735_LCD_HEIGHT)) return;
    if((x + w - 1) >= ST7735_LCD_WIDTH) w = ST7735_LCD_WIDTH - x;
    if((y + h - 1) >= ST7735_LCD_HEIGHT) h = ST7735_LCD_HEIGHT - y;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);

    uint8_t data[] = { color >> 8, color & 0xFF };
    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    for(y = h; y > 0; y--) {
        for(x = w; x > 0; x--) {
            HAL_SPI_Transmit(&ST7735_SPI_PORT, data, sizeof(data), HAL_MAX_DELAY);
        }
    }

    ST7735_Unselect();
}

void ST7735_FillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color) {
    // clipping
    if((x >= ST7735_LCD_WIDTH) || (y >= ST7735_LCD_HEIGHT)) return;
    if((x + w - 1) >= ST7735_LCD_WIDTH) w = ST7735_LCD_WIDTH - x;
    if((y + h - 1) >= ST7735_LCD_HEIGHT) h = ST7735_LCD_HEIGHT - y;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);

    // Prepare whole line in a single buffer
    uint8_t pixel[] = { color >> 8, color & 0xFF };
    uint8_t *line = malloc(w * sizeof(pixel));
    for(x = 0; x < w; ++x)
    	memcpy(line + x * sizeof(pixel), pixel, sizeof(pixel));

    HAL_GPIO_WritePin(ST7735_DC_GPIO_Port, ST7735_DC_Pin, GPIO_PIN_SET);
    for(y = h; y > 0; y--)
        HAL_SPI_Transmit(&ST7735_SPI_PORT, line, w * sizeof(pixel), HAL_MAX_DELAY);

    free(line);
    ST7735_Unselect();
}

void ST7735_FillScreen(uint16_t color) {
    ST7735_FillRectangle(0, 0, ST7735_LCD_WIDTH, ST7735_LCD_HEIGHT, color);
}

void ST7735_FillScreenFast(uint16_t color) {
    ST7735_FillRectangleFast(0, 0, ST7735_LCD_WIDTH, ST7735_LCD_HEIGHT, color);
}

void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data) {
    if((x >= ST7735_LCD_WIDTH) || (y >= ST7735_LCD_HEIGHT)) return;
    if((x + w - 1) >= ST7735_LCD_WIDTH) return;
    if((y + h - 1) >= ST7735_LCD_HEIGHT) return;

    ST7735_Select();
    ST7735_SetAddressWindow(x, y, x+w-1, y+h-1);
    ST7735_WriteData((uint8_t*)data, sizeof(uint16_t)*w*h);
    ST7735_Unselect();
}

void ST7735_InvertColors(bool invert) {
    ST7735_Select();
    ST7735_WriteCommand(invert ? ST7735_INVON : ST7735_INVOFF);
    ST7735_Unselect();
}

void ST7735_SetGamma(GammaDef gamma)
{
	ST7735_Select();
	ST7735_WriteCommand(ST7735_GAMSET);
	ST7735_WriteData((uint8_t *) &gamma, sizeof(gamma));
	ST7735_Unselect();
}



/* more function */

#define _swap_int16_t(a, b)                                                    \
  {                                                                            \
    int16_t t = a;                                                             \
    a = b;                                                                     \
    b = t;                                                                     \
  }

#define min(a, b) (((a) < (b)) ? (a) : (b))


void ST7735_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color)
{
	writeLine(x, y, x, y + h - 1, color);
}
void ST7735_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color)
{
	writeLine(x, y, x + w - 1, y, color);
}

void ST7735_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    if(x0 == x1){
        if(y0 > y1) _swap_int16_t(y0, y1);
        ST7735_DrawFastVLine(x0, y0, y1 - y0 + 1, color);
    } else if(y0 == y1){
        if(x0 > x1) _swap_int16_t(x0, x1);
        ST7735_DrawFastHLine(x0, y0, x1 - x0 + 1, color);
    } else {
        writeLine(x0, y0, x1, y1, color);
    }
}

void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t steep = abs(y1 - y0) > abs(x1 - x0);
    if (steep) {
        _swap_int16_t(x0, y0);
        _swap_int16_t(x1, y1);
    }

    if (x0 > x1) {
        _swap_int16_t(x0, x1);
        _swap_int16_t(y0, y1);
    }

    int16_t dx, dy;
    dx = x1 - x0;
    dy = abs(y1 - y0);

    int16_t err = dx / 2;
    int16_t ystep;

    if (y0 < y1) {
        ystep = 1;
    } else {
        ystep = -1;
    }

    for (; x0<=x1; x0++) {
        if (steep) {
        	ST7735_DrawPixel(y0, x0, color);
        } else {
        	ST7735_DrawPixel(x0, y0, color);
        }
        err -= dy;
        if (err < 0) {
            y0 += ystep;
            err += dx;
        }
    }
}

void ST7735_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    int16_t f = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x = 0;
    int16_t y = r;

    ST7735_DrawPixel(x0  , y0+r, color);
    ST7735_DrawPixel(x0  , y0-r, color);
    ST7735_DrawPixel(x0+r, y0  , color);
    ST7735_DrawPixel(x0-r, y0  , color);

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f += ddF_y;
        }
        x++;
        ddF_x += 2;
        f += ddF_x;

        ST7735_DrawPixel(x0 + x, y0 + y, color);
        ST7735_DrawPixel(x0 - x, y0 + y, color);
        ST7735_DrawPixel(x0 + x, y0 - y, color);
        ST7735_DrawPixel(x0 - x, y0 - y, color);
        ST7735_DrawPixel(x0 + y, y0 + x, color);
        ST7735_DrawPixel(x0 - y, y0 + x, color);
        ST7735_DrawPixel(x0 + y, y0 - x, color);
        ST7735_DrawPixel(x0 - y, y0 - x, color);
    }
}



void ST7735_DrawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color)
{
    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;

    while (x<y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        if (cornername & 0x4) {
            ST7735_DrawPixel(x0 + x, y0 + y, color);
            ST7735_DrawPixel(x0 + y, y0 + x, color);
        }
        if (cornername & 0x2) {
            ST7735_DrawPixel(x0 + x, y0 - y, color);
            ST7735_DrawPixel(x0 + y, y0 - x, color);
        }
        if (cornername & 0x8) {
            ST7735_DrawPixel(x0 - y, y0 + x, color);
            ST7735_DrawPixel(x0 - x, y0 + y, color);
        }
        if (cornername & 0x1) {
            ST7735_DrawPixel(x0 - y, y0 - x, color);
            ST7735_DrawPixel(x0 - x, y0 - y, color);
        }
    }
}

void ST7735_FillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color)
{

    int16_t f     = 1 - r;
    int16_t ddF_x = 1;
    int16_t ddF_y = -2 * r;
    int16_t x     = 0;
    int16_t y     = r;
    int16_t px    = x;
    int16_t py    = y;

    delta++; // Avoid some +1's in the loop

    while(x < y) {
        if (f >= 0) {
            y--;
            ddF_y += 2;
            f     += ddF_y;
        }
        x++;
        ddF_x += 2;
        f     += ddF_x;
        // These checks avoid double-drawing certain lines, important
        // for the SSD1306 library which has an INVERT drawing mode.
        if(x < (y + 1)) {
            if(corners & 1) ST7735_DrawFastVLine(x0+x, y0-y, 2*y+delta, color);
            if(corners & 2) ST7735_DrawFastVLine(x0-x, y0-y, 2*y+delta, color);
        }
        if(y != py) {
            if(corners & 1) ST7735_DrawFastVLine(x0+py, y0-px, 2*px+delta, color);
            if(corners & 2) ST7735_DrawFastVLine(x0-py, y0-px, 2*px+delta, color);
            py = y;
        }
        px = x;
    }
}

void ST7735_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color)
{
    ST7735_DrawFastVLine(x0, y0-r, 2*r+1, color);
    ST7735_FillCircleHelper(x0, y0, r, 3, 0, color);
}



void ST7735_DrawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color)
{
    ST7735_DrawFastHLine(x, y, w, color);
    ST7735_DrawFastHLine(x, y+h-1, w, color);
    ST7735_DrawFastVLine(x, y, h, color);
    ST7735_DrawFastVLine(x+w-1, y, h, color);
}

void ST7735_DrawRoundRectangle(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    ST7735_DrawFastHLine(x+r  , y    , w-2*r, color); // Top
    ST7735_DrawFastHLine(x+r  , y+h-1, w-2*r, color); // Bottom
    ST7735_DrawFastVLine(x    , y+r  , h-2*r, color); // Left
    ST7735_DrawFastVLine(x+w-1, y+r  , h-2*r, color); // Right
    // draw four corners
    ST7735_DrawCircleHelper(x+r    , y+r    , r, 1, color);
    ST7735_DrawCircleHelper(x+w-r-1, y+r    , r, 2, color);
    ST7735_DrawCircleHelper(x+w-r-1, y+h-r-1, r, 4, color);
    ST7735_DrawCircleHelper(x+r    , y+h-r-1, r, 8, color);
}


void ST7735_FillRoundRectangle(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color)
{
    int16_t max_radius = ((w < h) ? w : h) / 2; // 1/2 minor axis
    if(r > max_radius) r = max_radius;
    // smarter version
    ST7735_FillRectangle(x+r, y, w-2*r, h, color);
    // draw four corners
    ST7735_FillCircleHelper(x+w-r-1, y+r, r, 1, h-2*r-1, color);
    ST7735_FillCircleHelper(x+r    , y+r, r, 2, h-2*r-1, color);
}


void ST7735_DrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{
    ST7735_DrawLine(x0, y0, x1, y1, color);
    ST7735_DrawLine(x1, y1, x2, y2, color);
    ST7735_DrawLine(x2, y2, x0, y0, color);
}


void ST7735_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color)
{

    int16_t a, b, y, last;

    // Sort coordinates by Y order (y2 >= y1 >= y0)
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }
    if (y1 > y2) {
        _swap_int16_t(y2, y1); _swap_int16_t(x2, x1);
    }
    if (y0 > y1) {
        _swap_int16_t(y0, y1); _swap_int16_t(x0, x1);
    }

    if(y0 == y2) { // Handle awkward all-on-same-line case as its own thing
        a = b = x0;
        if(x1 < a)      a = x1;
        else if(x1 > b) b = x1;
        if(x2 < a)      a = x2;
        else if(x2 > b) b = x2;
        ST7735_DrawFastHLine(a, y0, b-a+1, color);
        return;
    }

    int16_t
    dx01 = x1 - x0,
    dy01 = y1 - y0,
    dx02 = x2 - x0,
    dy02 = y2 - y0,
    dx12 = x2 - x1,
    dy12 = y2 - y1;
    int32_t
    sa   = 0,
    sb   = 0;

    // For upper part of triangle, find scanline crossings for segments
    // 0-1 and 0-2.  If y1=y2 (flat-bottomed triangle), the scanline y1
    // is included here (and second loop will be skipped, avoiding a /0
    // error there), otherwise scanline y1 is skipped here and handled
    // in the second loop...which also avoids a /0 error here if y0=y1
    // (flat-topped triangle).
    if(y1 == y2) last = y1;   // Include y1 scanline
    else         last = y1-1; // Skip it

    for(y=y0; y<=last; y++) {
        a   = x0 + sa / dy01;
        b   = x0 + sb / dy02;
        sa += dx01;
        sb += dx02;
        /* longhand:
        a = x0 + (x1 - x0) * (y - y0) / (y1 - y0);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) _swap_int16_t(a,b);
        ST7735_DrawFastHLine(a, y, b-a+1, color);
    }

    // For lower part of triangle, find scanline crossings for segments
    // 0-2 and 1-2.  This loop is skipped if y1=y2.
    sa = (int32_t)dx12 * (y - y1);
    sb = (int32_t)dx02 * (y - y0);
    for(; y<=y2; y++) {
        a   = x1 + sa / dy12;
        b   = x0 + sb / dy02;
        sa += dx12;
        sb += dx02;
        /* longhand:
        a = x1 + (x2 - x1) * (y - y1) / (y2 - y1);
        b = x0 + (x2 - x0) * (y - y0) / (y2 - y0);
        */
        if(a > b) _swap_int16_t(a,b);
        ST7735_DrawFastHLine(a, y, b-a+1, color);
    }
}



/* TESTING */
void ST7735_TestLines(uint16_t color)
{
    int           x1, y1, x2, y2,
                  w = ST7735_LCD_WIDTH,
                  h = ST7735_LCD_HEIGHT;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);

    x1 = y1 = 0;
    y2    = h - 1;
    for (x2 = 0; x2 < w; x2 += 6) ST7735_DrawLine(x1, y1, x2, y2, color);
    x2    = w - 1;
    for (y2 = 0; y2 < h; y2 += 6) ST7735_DrawLine(x1, y1, x2, y2, color);

    ST7735_FillScreen(ST7735_COLOUR_BLACK);

    x1    = w - 1;
    y1    = 0;
    y2    = h - 1;
    for (x2 = 0; x2 < w; x2 += 6) ST7735_DrawLine(x1, y1, x2, y2, color);
    x2    = 0;
    for (y2 = 0; y2 < h; y2 += 6) ST7735_DrawLine(x1, y1, x2, y2, color);

    ST7735_FillScreen(ST7735_COLOUR_BLACK);

    x1    = 0;
    y1    = h - 1;
    y2    = 0;
    for (x2 = 0; x2 < w; x2 += 6) ST7735_DrawLine(x1, y1, x2, y2, color);
    x2    = w - 1;
    for (y2 = 0; y2 < h; y2 += 6) ST7735_DrawLine(x1, y1, x2, y2, color);

    ST7735_FillScreen(ST7735_COLOUR_BLACK);

    x1    = w - 1;
    y1    = h - 1;
    y2    = 0;
    for (x2 = 0; x2 < w; x2 += 6) ST7735_DrawLine(x1, y1, x2, y2, color);
    x2    = 0;
    for (y2 = 0; y2 < h; y2 += 6) ST7735_DrawLine(x1, y1, x2, y2, color);

}

void ST7735_TestFastLines(uint16_t color1, uint16_t color2)
{
    int           x, y, w = ST7735_LCD_WIDTH, h = ST7735_LCD_HEIGHT;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    for (y = 0; y < h; y += 5) ST7735_DrawFastHLine(0, y, w, color1);
    for (x = 0; x < w; x += 5) ST7735_DrawFastVLine(x, 0, h, color2);
}

void ST7735_TestRectangles(uint16_t color)
{
    int           n, i, i2,
                  cx = ST7735_LCD_WIDTH  / 2,
                  cy = ST7735_LCD_HEIGHT / 2;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    n     = min(ST7735_LCD_WIDTH, ST7735_LCD_HEIGHT);
    for (i = 2; i < n; i += 6) {
        i2 = i / 2;
        ST7735_DrawRectangle(cx - i2, cy - i2, i, i, color);
    }

}

void ST7735_TestFilledRectangles(uint16_t color1, uint16_t color2)
{
    int           n, i, i2,
                  cx = ST7735_LCD_WIDTH  / 2 - 1,
                  cy = ST7735_LCD_HEIGHT / 2 - 1;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    n = min(ST7735_LCD_WIDTH, ST7735_LCD_HEIGHT);
    for (i = n; i > 0; i -= 6) {
        i2    = i / 2;

        ST7735_FillRectangle(cx - i2, cy - i2, i, i, color1);

        ST7735_DrawRectangle(cx - i2, cy - i2, i, i, color2);
    }
}

void ST7735_TestFilledCircles(uint8_t radius, uint16_t color)
{
    int x, y, w = ST7735_LCD_WIDTH, h = ST7735_LCD_HEIGHT, r2 = radius * 2;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    for (x = radius; x < w; x += r2) {
        for (y = radius; y < h; y += r2) {
            ST7735_FillCircle(x, y, radius, color);
        }
    }

}

void ST7735_TestCircles(uint8_t radius, uint16_t color)
{
    int           x, y, r2 = radius * 2,
                        w = ST7735_LCD_WIDTH  + radius,
                        h = ST7735_LCD_HEIGHT + radius;

    // Screen is not cleared for this one -- this is
    // intentional and does not affect the reported time.
    for (x = 0; x < w; x += r2) {
        for (y = 0; y < h; y += r2) {
            ST7735_DrawCircle(x, y, radius, color);
        }
    }

}

void ST7735_TestTriangles()
{
    int           n, i, cx = ST7735_LCD_WIDTH  / 2 - 1,
                        cy = ST7735_LCD_HEIGHT / 2 - 1;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    n     = min(cx, cy);
    for (i = 0; i < n; i += 5) {
        ST7735_DrawTriangle(
            cx    , cy - i, // peak
            cx - i, cy + i, // bottom left
            cx + i, cy + i, // bottom right
            ST7735_COLOR565(0, 0, i));
    }

}

void ST7735_TestFilledTriangles() {
    int           i, cx = ST7735_LCD_WIDTH  / 2 - 1,
                     cy = ST7735_LCD_HEIGHT / 2 - 1;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    for (i = min(cx, cy); i > 10; i -= 5) {
    	ST7735_FillTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
    	                         ST7735_COLOR565(0, i, i));
    	ST7735_DrawTriangle(cx, cy - i, cx - i, cy + i, cx + i, cy + i,
    	                         ST7735_COLOR565(i, i, 0));
    }
}

void ST7735_TestRoundRectangles() {
    int           w, i, i2, red, step,
                  cx = ST7735_LCD_WIDTH  / 2 - 1,
                  cy = ST7735_LCD_HEIGHT / 2 - 1;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    w     = min(ST7735_LCD_WIDTH, ST7735_LCD_HEIGHT);
    red = 0;
    step = (256 * 6) / w;
    for (i = 0; i < w; i += 6) {
        i2 = i / 2;
        red += step;
        ST7735_DrawRoundRectangle(cx - i2, cy - i2, i, i, i / 8, ST7735_COLOR565(red, 0, 0));
    }

}

void ST7735_TestFilledRoundRectangles() {
    int           i, i2, green, step,
                  cx = ST7735_LCD_WIDTH  / 2 - 1,
                  cy = ST7735_LCD_HEIGHT / 2 - 1;

    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    green = 256;
    step = (256 * 6) / min(ST7735_LCD_WIDTH, ST7735_LCD_HEIGHT);
    for (i = min(ST7735_LCD_WIDTH, ST7735_LCD_HEIGHT); i > 20; i -= 6) {
        i2 = i / 2;
        green -= step;
        ST7735_FillRoundRectangle(cx - i2, cy - i2, i, i, i / 8, ST7735_COLOR565(0, green, 0));
    }

}
void ST7735_TestFillScreen()
{
    ST7735_FillScreen(ST7735_COLOUR_BLACK);
    ST7735_FillScreen(ST7735_COLOUR_RED);
    ST7735_FillScreen(ST7735_COLOUR_GREEN);
    ST7735_FillScreen(ST7735_COLOUR_BLUE);
    ST7735_FillScreen(ST7735_COLOUR_BLACK);
}

/* TEST all functions */
void ST7735_TestAll (void)
{
	ST7735_TestFillScreen();
	ST7735_TestLines(ST7735_COLOUR_CYAN);
	ST7735_TestFastLines(ST7735_COLOUR_RED, ST7735_COLOUR_BLUE);
	ST7735_TestRectangles(ST7735_COLOUR_GREEN);
	ST7735_TestFilledRectangles(ST7735_COLOUR_YELLOW, ST7735_COLOUR_MAGENTA);
	ST7735_TestFilledCircles(10, ST7735_COLOUR_MAGENTA);
	ST7735_TestCircles(10, ST7735_COLOUR_WHITE);
	ST7735_TestTriangles();
	ST7735_TestFilledTriangles();
	ST7735_TestRoundRectangles();
	ST7735_TestFilledRoundRectangles();
}

uint16_t _currentX=ST7735_XSTART, _currentY=ST7735_YSTART, _currentLineNum=1;

void ST7735_WriteDebugString(char* str) {
	_currentX=ST7735_XSTART;
	ST7735_FillRectangle(_currentX, _currentY, ST7735_LCD_WIDTH, Font_7x10.height, ST7735_COLOUR_BLACK);


	ST7735_Select();

    while(*str)
    {
        if(_currentY + Font_7x10.height >= ST7735_LCD_HEIGHT)
        {
        	ST7735_FillScreen(ST7735_COLOUR_BLACK);
        	_currentY=ST7735_YSTART;
        }
        if(_currentX + Font_7x10.width >= ST7735_LCD_WIDTH)
        {
        	break;
        	//_currentX = 0;
        }
        if(*str == '\n')
        {
        	_currentY = (_currentY ) + (Font_7x10.height);
        	_currentX=ST7735_XSTART;
        	break;
//        	str++;
//        	continue;
        }

        ST7735_WriteChar(_currentX, _currentY, *str, Font_7x10, ST7735_COLOUR_WHITE, ST7735_COLOUR_BLACK);
        _currentX += Font_7x10.width;
        str++;
    }

    //_currentY = (_currentY ) + (Font_7x10.height);
    ST7735_Unselect();
}

