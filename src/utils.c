#include "util.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <conio.h>

/*****************************************************************************/
// find next quote char
int nextQuote(char* buf, uint16_t start, uint16_t len)
{
    uint16_t i;
    for (i = start; i < len; i++)
    {
        if (buf[i] == '\"')
            return i;
    }
    return 0xFFFF;
}
/*****************************************************************************/
// find next space char
int nextSpace(char* buf, uint16_t start, uint16_t len)
{
    uint16_t i;
    for (i = start; i < len; i++)
    {
        if (buf[i] == ' ')
            return i;
    }
    return 0xFFFF;
}

/*****************************************************************************/
// find last space char
int lastSpace(char* buf, uint16_t start, uint16_t len)
{
    uint16_t i;
    for (i = start; i < len; i++)
    {
        if (buf[i] != ' ')
            return i;
    }
    return 0xFFFF;
}

/*****************************************************************************/
// dump memory
void dump(uint16_t addr, int len)
{
    uint16_t index = 0;
    uint8_t x = 0;
    uint8_t y = 0;
    uint8_t b = 0;

    for (index = 0; index < len; index++)
    {
        b = ((char*)addr + index)[0];
        x = index % 16 * 3;
        y = index / 16 + 25;
        if (x > 22)
            x++;
        gotoxy(x,y);
        printf("%02X", b);

        x = index % 16 + 50;
        y = index / 16 + 25;
        gotoxy(x,y);
        if (b >= 0x20 && b < 0x80 || b > 0xA0)
            cbm_k_chrout(b);

    }
    printf("\r\n");
}