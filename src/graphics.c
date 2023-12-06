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

//----------------------------------------------------------------------------

uint32_t spriteAttr0 = ((uint32_t)VERA_INC_1 << 16) | SPRITE_ATTR0;
#define SPRITE_ATTR_BASE(index) spriteAttr0 + (uint32_t)(index * 8)


//----------------------------------------------------------------------------
// load into VRAM
uint16_t veraload(const char *fileName, uint8_t device, uint32_t rawAddr)
{
    uint8_t bank = (uint8_t)(rawAddr >> 16) & 0xf;
    uint16_t addr = (uint16_t)(rawAddr & 0xffff); // baseAddr;

    cbm_k_setlfs(2,device,0);
    cbm_k_setnam(fileName);
    // Note: cbm_k_load() flag overloaded to specify the VERA bank + 2
    return (cbm_k_load(2+bank,addr) - addr);
}


//----------------------------------------------------------------------------
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
        res = veraload(filename, 8, addr);
    }
}

//----------------------------------------------------------------------------
void setSpriteBitmap(uint8_t index, uint32_t addr)
{
    uint32_t spriteAttrBaseAddr = SPRITE_ATTR_BASE(index);
    // Sprite attribute settings - memory increment set to 1

    SET_VERA_ADDR(spriteAttrBaseAddr);
    VERA.data0 = SPRITE_ADDR_L(addr);                   // Attr0
    VERA.data0 = SPRITE_ADDR_H(addr) | SPRITE_8BPP;     // Attr1
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void showSprite(uint8_t index)
{
    uint8_t flags = 0;
    uint32_t spriteAttrBaseAddr = SPRITE_ATTR_BASE(index);
    if (index == 0)
        return;    
    flags = vpeek(spriteAttrBaseAddr+6) & 0xF3;    
    vpoke(flags | SPRITE_LAYER1, spriteAttrBaseAddr+6);  
}

//----------------------------------------------------------------------------
void hideSprite(uint8_t index)
{
    uint8_t flags = 0;
    uint32_t spriteAttrBaseAddr = SPRITE_ATTR_BASE(index);
    if (index == 0)
        return;    
    flags = vpeek(spriteAttrBaseAddr+6) & 0xF3;    
    vpoke(flags | SPRITE_DISABLED, spriteAttrBaseAddr+6);  
}


//----------------------------------------------------------------------------
// setup screen configuration (bitmapped + text)
int SetupScreenMode()
{
    uint8_t x = 0;
    uint8_t y = 0;
    uint32_t i=0;
    uint8_t a = 0;
    uint8_t tv = 0;
    int result = 0;

    tv = get_tv();
    clrscr();
    restoreScreenmode();
    cbm_k_bsout(CH_FONT_LOWER);
    screen_set_charset(3);

    videomode(VIDEOMODE_80x30);
    VERA.display.border = 5;
    VERA.control = VERA.control | 0x03;

    // set border safe zone. Resulting area is 560 x 192
    VERA.display.vstart = (16 * 2)/2;
    VERA.display.vstop = (480 - 16*2)/2;
    VERA.display.hstart = (20 * 2)/4;
    VERA.display.hstop = (640 - 20*2)/4;
    VERA.control = VERA.control & 0x81;
    VERA.display.border = 194;


    cbm_k_bsout(CH_FONT_LOWER);
    screen_set_charset(3);


    // 
    // HACK: Set mouse cursor (Sprite 0) to be transparent (invisible)
    for (i = 0; i < 256; i++)
    {
        vpoke(0x00, MOUSE_CURSOR_SPRITE_ADDR+i);
    }
    //setSpriteBitmap(0, MOUSE_CURSOR_SPRITE_ADDR);

    // clear top 
    //vpoke(0x00, screenTextAddr + 256*2*32 - i);
    set_tv(tv); // preserve tv mode (vga or composite)

    clrscr();
    // arrow up symbol
    SET_VERA_ADDR(MAIN_FONT_ADDR+0x1e*8);
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b00011000;
    VERA.data0 = 0b00111100;
    VERA.data0 = 0b01111110;
    VERA.data0 = 0b11111111;
    VERA.data0 = 0b11111111;
    VERA.data0 = 0b00000000;

    // arrow down symbol 0x5E, which is mapped to 0x7E
    SET_VERA_ADDR(MAIN_FONT_ADDR+0x5e*8);
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b11111111;
    VERA.data0 = 0b11111111;
    VERA.data0 = 0b01111110;
    VERA.data0 = 0b00111100;
    VERA.data0 = 0b00011000;
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b00000000;

    // arrow right 0x6B, which is mapped to 0x7C
    SET_VERA_ADDR(MAIN_FONT_ADDR+0x6B*8);
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b01100000;
    VERA.data0 = 0b00111000;
    VERA.data0 = 0b00011110;
    VERA.data0 = 0b00111000;
    VERA.data0 = 0b01100000;
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b00000000;

    // arrow left 0x6C, which is mapped to 0x7D
    SET_VERA_ADDR(MAIN_FONT_ADDR+0x5D*8);
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b00000110;
    VERA.data0 = 0b00011100;
    VERA.data0 = 0b01111000;
    VERA.data0 = 0b00011100;
    VERA.data0 = 0b00000110;
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b00000000;

    // // arrow left 0x6C, which is mapped to 0x7D
    // SET_VERA_ADDR(MAIN_FONT_ADDR+0x5D*8);
    // VERA.data0 = 0b00000000;
    // VERA.data0 = 0b00011000;
    // VERA.data0 = 0b00111100;
    // VERA.data0 = 0b01111110;
    // VERA.data0 = 0b00011000;
    // VERA.data0 = 0b00011000;
    // VERA.data0 = 0b00011110;
    // VERA.data0 = 0b00000000;    

    // program icon to 0x5F, which is mapped to 0x7F
    SET_VERA_ADDR(MAIN_FONT_ADDR+0x5f*8);
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b01111111;
    VERA.data0 = 0b01000001;
    VERA.data0 = 0b01000001;
    VERA.data0 = 0b01000001;
    VERA.data0 = 0b01000001;
    VERA.data0 = 0b01111111;
    VERA.data0 = 0b00000000;
    VERA.data0 = 0b00000000;
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

//----------------------------------------------------------------------------
// restore screen mode back to boot state
void restoreScreenmode()
{
    uint8_t tv=get_tv();
    
    //screen_set_charset(2);
    //cbm_k_bsout(CH_FONT_UPPER);

    //hideAllSprites();
    __asm__("jsr $FF81"); // call CINT
    set_tv(tv);
}

//----------------------------------------------------------------------------
// hide all sprites
void hideAllSprites()
{
    uint8_t i;
    for (i = 0; i < 10; i++)
    {
        hideSprite(i);
    }   
}

//----------------------------------------------------------------------------
// show text with word wrap in a specific rectangle
void showTextWrapped(char* strText, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    int i=0;
    int p=0;
    int len=strlen(strText);
    uint8_t l = 0;
    uint8_t t = y;
    char c=0;
    char* szWord=strText;
    static char buf[40];

    gotoxy(x, y);                       // start top right coordinates

    for (i=0; i<=len; i++)
    {
        c=strText[i];
        if (c==' ' || c== '\r' || i >= len)
        {
            l = strText+i - szWord;     // length of word
            if (p+l > w)                // wrap around?
            {
                p=0;
                y++;
                gotoxy(x, y);
            }
            gotoxy(x+p, y);             // print word
            strncpy(buf, szWord, l);
            buf[l]=0;
            printf(buf);
            p+=l+1;                     // include space
            if (c=='\r')                // newline?
            {
                i++;
                p=0;
                y++;
                gotoxy(x, y);
            }
            else
                printf(" ");            // include space
                if (y > t+h)            // exceeding max lines?
                    return;

            szWord = strText+i+1;       // jump to next word
        }
    }

}