#ifndef _GRAPHICS_H
#define _GRAPHICS_H

#include <stdint.h>
#include "util.h"

#define LAYER0 0b01
#define LAYER1 0b10

#define MAIN_SCREEN_BMP_ADDR 0x08000
#define MAIN_SCREEN_TXT_ADDR 0x1B000
#define MAIN_FONT_ADDR       0x1F000

#define THUMBNAIL_BUFFER_ADDR      0x04000
#define THUMBNAIL_BASE_ADDR      0x08000
#define MOUSE_CURSOR_SPRITE_ADDR        0x1EC00
#define SET_VERA_ADDR(addr) VERA.address_hi = (uint8_t)VERA_INC_1  | ((addr >> 16) & 0x0F); VERA.address = (uint16_t)addr & 0xFFFF;
#define SET_VERA_ADDR_SHORT(addr) VERA.address = addr;

void createSprite(uint8_t index, uint8_t width, uint8_t height, uint16_t xPos, uint16_t yPos, uint32_t addr, const char* filename);
void setSpriteBitmap(uint8_t index, uint32_t addr);
void setSpritePosition(uint8_t index, uint16_t px, uint16_t py);
void showSprite(uint8_t index);
void hideSprite(uint8_t index);
void hideAllSprites();
int SetupScreenMode();
void restoreScreenmode();
uint16_t veraload(const char* filename, uint8_t device, uint32_t addr);
void showTextWrapped(char* strText, uint8_t x, uint8_t y, uint8_t w, uint8_t h);

#endif