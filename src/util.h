#ifndef UTIL_H
#define UTIL_H

#include <stdint.h>

extern void __fastcall__ vsync();
extern int16_t __fastcall__ cbm_k_macptr(unsigned char numbyte, uint8_t* buffer);
extern void screen_set_charset(unsigned char charset);
extern int8_t check_dos_error();
extern void split_thumbnail();

extern uint16_t s_addr;
extern uint16_t s_offset;

extern char dosmsg[80];
#endif