#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <conio.h>
#include <cbm.h>
#include <unistd.h>
#include <cx16.h>
#include <vera.h>
#include <vload.h>

#include "graphics.h"

uint32_t spriteAttr0 = ((uint32_t)VERA_INC_1 << 16) | SPRITE_ATTR0;
#define SPRITE_ATTR_BASE(index) spriteAttr0 + (uint32_t)(index * 8)


/*****************************************************************************/
// load into VRAM
uint16_t vload(const char *fileName, uint8_t device, uint32_t rawAddr)
{
    uint8_t bank = (uint8_t)(rawAddr >> 16) & 0xf;
    uint16_t addr = (uint16_t)(rawAddr & 0xffff); // baseAddr;

    cbm_k_setlfs(1,device,0);
    cbm_k_setnam(fileName);
    // Note: cbm_k_load() flag overloaded to specify the VERA bank + 2
    return (cbm_k_load(2+bank,addr) - addr);
}


/*****************************************************************************/
// create and load a sprite
void createSprite(uint8_t index, uint8_t width, uint8_t height, uint16_t xPos, uint16_t yPos, uint32_t addr, const char* filename)
{
    uint8_t sprite_w = SPRITE_WIDTH_16;
    uint8_t sprite_h = SPRITE_HEIGHT_16;
    int16_t res = 0;

    switch(width)
    {
        case 8: sprite_w = SPRITE_WIDTH_8; break;
        case 16: sprite_w = SPRITE_WIDTH_16; break;
        case 32: sprite_w = SPRITE_WIDTH_32; break;
        case 64: sprite_w = SPRITE_WIDTH_64; break;
        default: return;
    }
    switch(height)
    {
        case 8: sprite_h = SPRITE_HEIGHT_8; break;
        case 16: sprite_h = SPRITE_HEIGHT_16; break;
        case 32: sprite_h = SPRITE_HEIGHT_32; break;
        case 64: sprite_h = SPRITE_HEIGHT_64; break;
        default: return;
    }
    
    // Sprite attribute settings - memory increment set to 1
    SET_VERA_ADDR(SPRITE_ATTR_BASE(index));
    VERA.data0 = SPRITE_ADDR_L(addr);                   // Attr0
    VERA.data0 = SPRITE_ADDR_H(addr) | SPRITE_8BPP;     // Attr1
    VERA.data0 = SPRITE_X_L(xPos);                      // Attr2
    VERA.data0 = SPRITE_X_H(xPos);                      // Attr3
    VERA.data0 = SPRITE_Y_L(yPos);                      // Attr4
    VERA.data0 = SPRITE_Y_H(yPos);                      // Attr5
    VERA.data0 = SPRITE_LAYER1;                         // Attr6
    VERA.data0 = sprite_h | sprite_w;                        // Attr7

    if (filename)
    {
        // Copy the player sprite data into the video RAM.
        // Set the address to increment with each access.
        res = vload(filename, 8, addr);
    }
}

/*****************************************************************************/
void setSpriteBitmap(uint8_t index, uint32_t addr)
{
    uint32_t spriteAttrBaseAddr = SPRITE_ATTR_BASE(index);
    // Sprite attribute settings - memory increment set to 1

    SET_VERA_ADDR(spriteAttrBaseAddr);
    VERA.data0 = SPRITE_ADDR_L(addr);                   // Attr0
    VERA.data0 = SPRITE_ADDR_H(addr) | SPRITE_8BPP;     // Attr1
}

/*****************************************************************************/
// set pixel pos for sprite
void setSpritePosition(uint8_t index, uint16_t px, uint16_t py)
{
    uint32_t spriteAttrBaseAddr = SPRITE_ATTR_BASE(index);

    // set sprite position
    SET_VERA_ADDR(spriteAttrBaseAddr+2);
    VERA.data0 = SPRITE_X_L(px);                 // Attr2
    VERA.data0 = SPRITE_X_H(px);                 // Attr3
    VERA.data0 = SPRITE_Y_L(py);                 // Attr4
    // OPTIMIZED: VERA.data0 = SPRITE_Y_H(py);                 // Attr5
}

/*****************************************************************************/
void showSprite(uint8_t index)
{
    uint8_t flags = 0;
    uint32_t spriteAttrBaseAddr = SPRITE_ATTR_BASE(index);
    if (index == 0)
        return;    
    flags = vpeek(spriteAttrBaseAddr+6) & 0xF3;    
    vpoke(flags | SPRITE_LAYER1, spriteAttrBaseAddr+6);  
}

/*****************************************************************************/
void hideSprite(uint8_t index)
{
    uint8_t flags = 0;
    uint32_t spriteAttrBaseAddr = SPRITE_ATTR_BASE(index);
    if (index == 0)
        return;    
    flags = vpeek(spriteAttrBaseAddr+6) & 0xF3;    
    vpoke(flags | SPRITE_DISABLED, spriteAttrBaseAddr+6);  
}


/*****************************************************************************/
// setup screen configuration (bitmapped + text)
int SetupScreenMode()
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint32_t i=0;
    uint8_t a = 0;
    int result = 0;

    videomode(VIDEOMODE_80x30);
    VERA.display.border = 5;
    VERA.control = VERA.control | 0x03;

/*
    VERA.layer1.config = MAP_WIDTH_128 | MAP_HEIGHT_32 | LAYER_CONFIG_4BPP | 0x08;              // 128x64 map, color depth 1, 256 color text mode
    VERA.layer1.mapbase = MAP_BASE_ADDR(screenTextAddr);                                        // Map base at 0x00000 (text content/color)
    VERA.layer1.tilebase = TILE_BASE_ADDR(screenFontAddr) | TILE_WIDTH_8 | TILE_HEIGHT_8;       // Tile base in this case char set
*/
    // set border safe zone. Resulting area is 560 x 192
    VERA.display.vstart = (16 * 2)/2;
    VERA.display.vstop = (480 - 16*2)/2;
    VERA.display.hstart = (20 * 2)/4;
    VERA.display.hstop = (640 - 20*2)/4;
    VERA.control = VERA.control & 0x81;
    VERA.display.border = 194;


    cbm_k_bsout(CH_FONT_LOWER);
    clrscr();

    // 
    // HACK: Set mouse cursor (Sprite 0) to be transparent (invisible)
    for (i = 0; i < 256; i++)
    {
        vpoke(0x00, MOUSE_CURSOR_SPRITE_ADDR+i);
    }
    //setSpriteBitmap(0, MOUSE_CURSOR_SPRITE_ADDR);

    // clear top 
    //vpoke(0x00, screenTextAddr + 256*2*32 - i);

    clrscr();
/*// show palette
    for (y=0;y<16;y++)
    {
        for(x=0;x<16;x++)
        {
            i = y*128*2+x*2;
            a=y*16+x;
            //vpoke(a, i);
            //vpoke(a%2?255:0xFA, i+1);
            vpoke(screenTextAddr + 128+32, i);
            vpoke(screenTextAddr + y*16+x, i+1);
        }
    }
*/
    vera_sprites_enable(true);

    return 1;

}

/*****************************************************************************/
// hide all sprites
void hideAllSprites()
{
    uint8_t i;
    for (i = 0; i < 10; i++)
    {
        hideSprite(i);
    }   
}