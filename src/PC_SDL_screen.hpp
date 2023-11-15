#pragma once

#ifdef PC
// Include guard PC

#include <cstdint>

void setPixel(int x, int y, uint32_t color);

void setPixel_Unsafe(int x, int y, uint32_t color);

void LCD_ClearScreen();

// Unlike ClassPad, SDL2 can render all colors as 8bit (24b colors + 8b alpha).
uint32_t color(uint8_t R, uint8_t G, uint8_t B);

void fillScreen(uint32_t color);

//Draw a line (bresanham line algorithm)
void line(int x1, int y1, int x2, int y2, uint32_t color);
void vline(int x, int y1, int y2, uint32_t color);

//Draw a filled triangle.
void triangle(int x0, int y0, int x1, int y1, int x2, int y2, uint32_t colorFill, uint32_t colorLine);

int drawCharacter(char character, int x, int y, uint32_t* screenPixels);

void sdl_debug_uint32_t(uint32_t value, int x, int y);

// Include guard PC
#endif // PC