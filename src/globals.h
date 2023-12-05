#ifndef _GLOBALS_H
#define _GLOBALS_H

#include <stdint.h>
#include "dirtools.h"

//----------------------------------------------------------------------------
#define TMP_FILE ".launcher.tmp"
#define INI_FILE "launcher.ini"

//----------------------------------------------------------------------------
extern uint8_t res1;
extern uint8_t res2;
extern uint8_t res3;
extern uint16_t VarTab;
extern char* Name;

//----------------------------------------------------------------------------
extern struct DirCollection DirList;
extern struct DirCollection SubList;

extern struct DirEntry arrDirectory[];
extern uint8_t numEntries;
extern uint8_t maxEntries;

extern int8_t pageSize;
extern int8_t pageStart;
extern int8_t pageEnd;

extern char baseDir[];
extern char startDir[];
extern char thumbDir[];
extern char metaDir[];

extern bool isLocalMode;

#endif