/*
 * VERSION: R1.0
 * ST7735 BASED SPI LCD DRIVER LIBRARY
 * This library is a mix of two libraries from the below git paths
 * 	1. https://github.com/afiskon/stm32-st7735.git
 * 	2. https://controllerstech.com/st7735-1-8-tft-display-with-stm32/
 * 	I have included some functions from the second library and made a single one
 * 	It works with any rotation of screen and is optimized with usage of variables and functions
 */
#ifndef __ST7735_H__
#define __ST7735_H__

#include <stdbool.h>
#include "fonts.h"


#define ST7735_MADCTL_MY  0x80
#define ST7735_MADCTL_MX  0x40
#define ST7735_MADCTL_MV  0x20
#define ST7735_MADCTL_ML  0x10
#define ST7735_MADCTL_RGB 0x00
#define ST7735_MADCTL_BGR 0x08
#define ST7735_MADCTL_MH  0x04

/*
 * CHANGE AS PER YOUR BOARD - BEGIN
 */
#include "stm32f4xx_hal.h"
#define ST7735_SPI_PORT hspi1


#define ST7735_RES_Pin       GPIO_PIN_1
#define ST7735_RES_GPIO_Port GPIOB
#define ST7735_CS_Pin        GPIO_PIN_2
#define ST7735_CS_GPIO_Port  GPIOB
#define ST7735_DC_Pin        GPIO_PIN_0
#define ST7735_DC_GPIO_Port  GPIOB

/*
 * CHANGE AS PER YOUR BOARD - END
 */

/* SPI Handler is linked to the variable */

extern SPI_HandleTypeDef ST7735_SPI_PORT;

/* =====================================================================
 * Below DISPLAY formats are for various orientations. UNCOMMENT a type and use it
 * (Dont forget to comment the remaining)
 */
// AliExpress/eBay 1.8" display, default orientation
/*
#define ST7735_LCD_IS_160X128 1
#define ST7735_LCD_WIDTH  128
#define ST7735_LCD_HEIGHT 160
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY)
*/

// AliExpress/eBay 1.8" display, rotate right
/*
#define ST7735_LCD_IS_160X128 1
#define ST7735_LCD_WIDTH  160
#define ST7735_LCD_HEIGHT 128
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV)
*/

// AliExpress/eBay 1.8" display, rotate left
/*
#define ST7735_LCD_IS_160X128 1
#define ST7735_LCD_WIDTH  160
#define ST7735_LCD_HEIGHT 128
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MV)
*/

// AliExpress/eBay 1.8" display, upside down
/*
#define ST7735_LCD_IS_160X128 1
#define ST7735_LCD_WIDTH  128
#define ST7735_LCD_HEIGHT 160
#define ST7735_XSTART 0
#define ST7735_YSTART 0
#define ST7735_LCD_ROTATION (0)
*/

// WaveShare ST7735S-based 1.8" display, default orientation
/*
#define ST7735_LCD_IS_160X128 1
#define ST7735_LCD_WIDTH  128
#define ST7735_LCD_HEIGHT 160
#define ST7735_XSTART 2
#define ST7735_YSTART 1
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_RGB)
*/

// WaveShare ST7735S-based 1.8" display, rotate right

#define ST7735_LCD_IS_160X128 1
#define ST7735_LCD_WIDTH  160
#define ST7735_LCD_HEIGHT 128
#define ST7735_XSTART 1
#define ST7735_YSTART 2
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_RGB)
#define ST7735_NUMBER_OF_CHARACTERS 22		//22 characters of smallest font.
#define ST7735_NUMBER_OF_LINES	12
#define ST7735_VERTICAL_TEXT_GAP	5
// WaveShare ST7735S-based 1.8" display, rotate left
/*
#define ST7735_LCD_IS_160X128 1
#define ST7735_LCD_WIDTH  160
#define ST7735_LCD_HEIGHT 128
#define ST7735_XSTART 1
#define ST7735_YSTART 2
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_RGB)
*/

// WaveShare ST7735S-based 1.8" display, upside down
/*
#define ST7735_LCD_IS_160X128 1
#define ST7735_LCD_WIDTH  128
#define ST7735_LCD_HEIGHT 160
#define ST7735_XSTART 2
#define ST7735_YSTART 1
#define ST7735_LCD_ROTATION (ST7735_MADCTL_RGB)
*/

// 1.44" display, default orientation
/*
#define ST7735_LCD_IS_128X128 1
#define ST7735_LCD_WIDTH  128
#define ST7735_LCD_HEIGHT 128
#define ST7735_XSTART 2
#define ST7735_YSTART 3
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR)
*/
// 1.44" display, rotate right
/*
#define ST7735_LCD_IS_128X128 1
#define ST7735_LCD_WIDTH  128
#define ST7735_LCD_HEIGHT 128
#define ST7735_XSTART 3
#define ST7735_YSTART 2
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR)
*/

// 1.44" display, rotate left
/*
#define ST7735_LCD_IS_128X128 1
#define ST7735_LCD_WIDTH  128
#define ST7735_LCD_HEIGHT 128
#define ST7735_XSTART 1
#define ST7735_YSTART 2
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_BGR)
*/

// 1.44" display, upside down
/*
#define ST7735_LCD_IS_128X128 1
#define ST7735_LCD_WIDTH  128
#define ST7735_LCD_HEIGHT 128
#define ST7735_XSTART 2
#define ST7735_YSTART 1
#define ST7735_LCD_ROTATION (ST7735_MADCTL_BGR)
*/

// mini 160x80 display (it's unlikely you want the default orientation)
/*
#define ST7735_LCD_IS_160X80 1
#define ST7735_XSTART 26
#define ST7735_YSTART 1
#define ST7735_LCD_WIDTH  80
#define ST7735_LCD_HEIGHT 160
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MY | ST7735_MADCTL_BGR)
*/

// mini 160x80, rotate left
/*
#define ST7735_LCD_IS_160X80 1
#define ST7735_XSTART 1
#define ST7735_YSTART 26
#define ST7735_LCD_WIDTH  160
#define ST7735_LCD_HEIGHT 80
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MX | ST7735_MADCTL_MV | ST7735_MADCTL_BGR)
*/

// mini 160x80, rotate right 
/*
#define ST7735_LCD_IS_160X80 1
#define ST7735_XSTART 1
#define ST7735_YSTART 26
#define ST7735_LCD_WIDTH  160
#define ST7735_LCD_HEIGHT 80
#define ST7735_LCD_ROTATION (ST7735_MADCTL_MY | ST7735_MADCTL_MV | ST7735_MADCTL_BGR)
*/

/****************************/

#define ST7735_NOP     0x00
#define ST7735_SWRESET 0x01
#define ST7735_RDDID   0x04
#define ST7735_RDDST   0x09

#define ST7735_SLPIN   0x10
#define ST7735_SLPOUT  0x11
#define ST7735_PTLON   0x12
#define ST7735_NORON   0x13

#define ST7735_INVOFF  0x20
#define ST7735_INVON   0x21
#define ST7735_GAMSET  0x26
#define ST7735_DISPOFF 0x28
#define ST7735_DISPON  0x29
#define ST7735_CASET   0x2A
#define ST7735_RASET   0x2B
#define ST7735_RAMWR   0x2C
#define ST7735_RAMRD   0x2E

#define ST7735_PTLAR   0x30
#define ST7735_COLMOD  0x3A
#define ST7735_MADCTL  0x36

#define ST7735_FRMCTR1 0xB1
#define ST7735_FRMCTR2 0xB2
#define ST7735_FRMCTR3 0xB3
#define ST7735_INVCTR  0xB4
#define ST7735_DISSET5 0xB6

#define ST7735_PWCTR1  0xC0
#define ST7735_PWCTR2  0xC1
#define ST7735_PWCTR3  0xC2
#define ST7735_PWCTR4  0xC3
#define ST7735_PWCTR5  0xC4
#define ST7735_VMCTR1  0xC5

#define ST7735_RDID1   0xDA
#define ST7735_RDID2   0xDB
#define ST7735_RDID3   0xDC
#define ST7735_RDID4   0xDD

#define ST7735_PWCTR6  0xFC

#define ST7735_GMCTRP1 0xE0
#define ST7735_GMCTRN1 0xE1

// Color definitions
#define	ST7735_COLOUR_BLACK   0x0000
#define	ST7735_COLOUR_BLUE    0x001F
#define	ST7735_COLOUR_RED     0xF800
#define	ST7735_COLOUR_GREEN   0x07E0
#define ST7735_COLOUR_CYAN    0x07FF
#define ST7735_COLOUR_MAGENTA 0xF81F
#define ST7735_COLOUR_YELLOW  0xFFE0
#define ST7735_COLOUR_WHITE   0xFFFF
#define ST7735_COLOR565(r, g, b) (((r & 0xF8) << 8) | ((g & 0xFC) << 3) | ((b & 0xF8) >> 3))

typedef enum {
	GAMMA_10 = 0x01,
	GAMMA_25 = 0x02,
	GAMMA_22 = 0x04,
	GAMMA_18 = 0x08
} GammaDef;

#ifdef __cplusplus
extern "C" {
#endif

// call before initializing any SPI devices
void ST7735_Unselect();

void ST7735_Init(void);
void ST7735_DrawPixel(uint16_t x, uint16_t y, uint16_t color);
void ST7735_WriteString(uint16_t x, uint16_t y, const char* str, FontDef font, uint16_t color, uint16_t bgcolor);
void ST7735_FillRectangle(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7735_FillRectangleFast(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void ST7735_FillScreen(uint16_t color);
void ST7735_FillScreenFast(uint16_t color);
void ST7735_DrawImage(uint16_t x, uint16_t y, uint16_t w, uint16_t h, const uint16_t* data);
void ST7735_InvertColors(bool invert);
void ST7735_SetGamma(GammaDef gamma);

/* FROM OTHER LIBRARY */
void ST7735_DrawFastVLine(int16_t x, int16_t y, int16_t h, uint16_t color);
void ST7735_DrawFastHLine(int16_t x, int16_t y, int16_t w, uint16_t color);
void ST7735_DrawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void ST7735_DrawRectangle(int16_t x, int16_t y, int16_t w, int16_t h, uint16_t color);
void ST7735_DrawRoundRectangle(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);
void ST7735_FillRoundRectangle(int16_t x, int16_t y, int16_t w, int16_t h, int16_t r, uint16_t color);

void ST7735_DrawTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);
void ST7735_FillTriangle(int16_t x0, int16_t y0, int16_t x1, int16_t y1, int16_t x2, int16_t y2, uint16_t color);

void ST7735_DrawCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);
void ST7735_FillCircle(int16_t x0, int16_t y0, int16_t r, uint16_t color);

void ST7735_DrawCircleHelper( int16_t x0, int16_t y0, int16_t r, uint8_t cornername, uint16_t color);
void ST7735_FillCircleHelper(int16_t x0, int16_t y0, int16_t r, uint8_t corners, int16_t delta, uint16_t color);
void ST7735_TestLines(uint16_t color);
void ST7735_TestFastLines(uint16_t color1, uint16_t color2);
void ST7735_TestRects(uint16_t color) ;
void ST7735_TestFilledRects(uint16_t color1, uint16_t color2);
void ST7735_TestFilledCircles(uint8_t radius, uint16_t color);
void ST7735_TestCircles(uint8_t radius, uint16_t color);
void writeLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void ST7735_TestTriangles();
void ST7735_TestFilledTriangles();
void ST7735_TestRoundRectangles();
void ST7735_TestFilledRoundRectangles();
void ST7735_TestFillScreen();
void ST7735_TestAll (void);
void ST7735_WriteDebugString(char* str);

#ifdef __cplusplus
}
#endif

#endif // __ST7735_H__
