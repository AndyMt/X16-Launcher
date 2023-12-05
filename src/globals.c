#include "globals.h"
#include <stdint.h>
#include "dirtools.h"


//----------------------------------------------------------------------------
extern uint8_t res1;
extern uint8_t res2;
extern uint8_t res3;
extern uint16_t VarTab;
extern char* Name;

//----------------------------------------------------------------------------
struct DirCollection DirList;
struct DirCollection SubList;

struct DirEntry arrDirectory[50];
uint8_t numEntries;
uint8_t maxEntries = 50;

int8_t pageSize = 20;
int8_t pageStart = 0;
int8_t pageEnd = 0;

char baseDir[40];
char startDir[40];
char thumbDir[40];
char metaDir[40];

bool isLocalMode = false;