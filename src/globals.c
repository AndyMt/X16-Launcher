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
static struct DirCollection DirList;
static struct DirCollection SubList;

static struct DirEntry arrDirectory[50];
uint8_t numEntries;
uint8_t maxEntries = 50;

static int8_t pageSize = 20;
static int8_t pageStart = 0;
static int8_t pageEnd = 0;

static char baseDir[40];
static char startDir[40];
static char launcherDir[40];

bool isLocalMode = false;