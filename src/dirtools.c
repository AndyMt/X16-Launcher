#include "dirtools.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <cx16.h>
#include <conio.h>
#include <cbm.h>
#include "util.h"

/*****************************************************************************/
// loads with current HiRAM bank = the one we want.
// Even if it's not, it's the one we're gonna get! :)
uint16_t bload(char* filename, uint16_t address) 
{
	cbm_k_setlfs(0,8,2);
	cbm_k_setnam(filename);
	return cbm_k_load(0,address)-address;
}

/*****************************************************************************/
// check if file exists
bool checkFile(char* filename)
{
    int res = 0;
    int lfn = 2;
    int sad = 2;
    int i=0;
    char c = 0;
    char status[41];
    uint8_t err = 0;

 //gotoxy(0,0);
 //printf(filename);
 //printf("          ");
    res = cbm_open(lfn,8,sad,filename);
    cbm_close(lfn);
    if (res == 0)
    {
        // open command channel to read status
        res = cbm_open(15, 8, 15, "");
        if (res == 0)
        {

            res = cbm_k_chkin(15);
            while (true)
            {
                c = cbm_k_getin();
                if (cbm_k_readst() || c==0x0D || i>=40)
                    break;
                status[i] = c;
                i++;
            }
            status[i] = 0;
            cbm_close(15);
gotoxy(0,1);
//printf(status);
printf(" "); // this is a strange hack which makes this function work... otherwise it hangs...

            return status[0]=='0' && status[1]=='0';
        } 
    }
// gotoxy(0,1);
// printf("NONE");
// printf("        ");
    return 0;
}

/*****************************************************************************/
// get directory
bool setupDirectory(struct DirCollection* dir, int max)
{
    dir->maxEntries = max;
    dir->numEntries = 0;
    dir->arrEntries = calloc(max, sizeof(struct DirEntry));
    return (bool)dir->arrEntries;
}

/*****************************************************************************/
// get directory
char* getCurrentDirectory()
{
    uint16_t len = 0;
    uint16_t pos = 0;
    uint16_t start = 0;
    uint16_t index = 0;
    uint16_t count = 0;
    char* pBuffer = (char*)(0xA000);
    static char name[32];
    static char type[8];
    static char strDirLoad[85];
    bool isDir = false;
    bool isPrg = false;
    bool isValid = false;

    strcpy(strDirLoad, "$=c");

    setHighBank(2);
    len = bload(strDirLoad, 0xA000);
    strcpy(name, "/");

    //dump(0xA000, 64);

    while (pos < len)
    {
        // get name
        pos = nextQuote(pBuffer, pos, len);
        if (pos == 0xFFFF)
            break;
        start = pos+1;
        pos = nextQuote(pBuffer, start, len);
        count = pos-start;
        if (count > 30)
            count = 30;
        memcpy(name, pBuffer+start, count);
        name[count] = 0;
        if (pBuffer[pos-1] != '\"')
            pos = nextQuote(pBuffer, pos, len);

        // get type (either PRG or DIR)
        pos = lastSpace(pBuffer, pos+1, len);
        start = pos;
        pos = nextSpace(pBuffer, start, len);
        count = pos-start;
        if (count > 30)
            count = 30;
        memcpy(type, pBuffer+start, count);
        type[count] = 0;

        isPrg = (bool)strstr(name, ".prg");
        isDir = type[0] == 'd';
        if (isDir)
        {
            return name;
        }
    }    

    return name;    
}

/*****************************************************************************/
// get directory
uint8_t getDirectory(struct DirCollection* dir, char* filter)
{
    uint16_t len = 0;
    uint16_t pos = 0;
    uint16_t start = 0;
    uint16_t index = 0;
    uint16_t count = 0;
    char* pBuffer = (char*)(0xA000);
    static char name[32];
    static char type[8];
    static char strDirLoad[85];
    bool isDir = false;
    bool isPrg = false;
    bool isValid = false;

    if (filter[0]=='d')
        strcpy(strDirLoad, "$");
    else if (filter[0]=='p')
        strcpy(strDirLoad, "$:*=p");
    else
        strcpy(strDirLoad, "$");

    dir->numEntries = 0;
    setHighBank(2);
    len = bload(strDirLoad, 0xA000);

    while (pos < len)
    {
        // get name
        pos = nextQuote(pBuffer, pos, len);
        if (pos == 0xFFFF)
            break;
        start = pos+1;
        pos = nextQuote(pBuffer, start, len);
        count = pos-start;
        if (count > 30)
            count = 30;
        memcpy(name, pBuffer+start, count);
        name[count] = 0;
        if (pBuffer[pos-1] != '\"')
            pos = nextQuote(pBuffer, pos, len);

        // get type (either PRG or DIR)
        pos = lastSpace(pBuffer, pos+1, len);
        start = pos;
        pos = nextSpace(pBuffer, start, len);
        count = pos-start;
        if (count > 30)
            count = 30;
        memcpy(type, pBuffer+start, count);
        type[count] = 0;

        isPrg = (bool)strstr(name, ".prg");
        isDir = type[0] == 'd';

        dir->arrEntries[index].isPrg = isPrg;
        dir->arrEntries[index].hasPrg = false;

        if (filter[0]=='p' && !isPrg)
            continue;
        else if (filter[0]=='d' && !isDir)
            continue;

        // only take valid entries and directories
        if (name[1] != 0 && (isDir || isValid || isPrg))
        {
            if (isPrg)
            {
                strcpy(type, "prg");
                dir->arrEntries[index].hasPrg = false;
            }
            else
            {
                dir->arrEntries[index].hasPrg = false;
            }

            strcpy(dir->arrEntries[index].name, name);
            strcpy(dir->arrEntries[index].type, type);

            dir->arrEntries[index].isDir = isDir;

            index++;
            dir->numEntries = index;
        }
    }    

    return index;    
}

/*****************************************************************************/
// change directory
bool changeDir(char* directory)
{
    int res = 0;
    int lfn = 15;
    int sad = 15;
    char command[80];

    strcpy(command,"cd:");
    strcat(command, directory);

//checkFile(directory);
    res = cbm_open(lfn,8,sad,command);
    cbm_k_close(lfn);
//checkFile(directory);

    return res == 4;    
}

