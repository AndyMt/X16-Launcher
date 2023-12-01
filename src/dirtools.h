#ifndef _DIRTOOLS_H
#define _DIRTOOLS_H

#include <stdint.h>
#include <stdbool.h>

/*****************************************************************************/

struct DirEntry
{
    char name[32];
    char type[6];
    bool isPrg;
    bool hasPrg;
    bool hasThumb;
    bool hasMeta;
    bool isDir;
};

struct DirCollection
{
    uint8_t numEntries;
    uint8_t maxEntries;
    struct DirEntry* arrEntries;
};

/*****************************************************************************/

extern uint16_t bload(char* filename, uint16_t address);
extern bool checkFile(char* filename);
extern bool setupDirectory(struct DirCollection* dir, int max);
extern uint8_t getDirectory(struct DirCollection* dir, char* filter);
extern char* getCurrentDirectory();
extern bool changeDir(char* directory);

#endif