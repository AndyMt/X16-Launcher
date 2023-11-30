#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>
#include <cx16.h>

/*****************************************************************************/
// set RAM bank
#define setHighBank(bank) RAM_BANK = bank;

extern void vsync();
extern int16_t cbm_k_macptr(unsigned char numbyte, uint8_t* buffer);
extern void screen_set_charset(unsigned char charset);
extern void screen_put_char(unsigned char c);
extern int8_t check_dos_error();
extern void split_thumbnail();

extern int nextQuote(char* buf, uint16_t start, uint16_t len);
extern int nextSpace(char* buf, uint16_t start, uint16_t len);
extern int lastSpace(char* buf, uint16_t start, uint16_t len);
void dump(uint16_t addr, int len);
void waitKeypress();

extern uint16_t s_addr;
extern uint16_t s_offset;

extern char dosmsg[80];
#endif