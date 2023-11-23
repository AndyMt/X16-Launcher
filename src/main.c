#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <cx16.h>
#include <conio.h>
#include <cbm.h>
#include <dirent.h>
#include <errno.h>
#include <joystick.h>
#include <vload.h>
#include "graphics.h"
#include "util.h"

/*****************************************************************************/

extern void LaunchPrg();
extern bool changeDir(char* directory);

extern uint8_t res1;
extern uint8_t res2;
extern uint8_t res3;
extern uint16_t VarTab;
extern char* Name;

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

static struct DirCollection DirList;
static struct DirCollection SubList;

static struct DirEntry arrDirectory[50];
uint8_t numEntries;
uint8_t maxEntries = 50;

static int8_t pageSize = 20;
static int8_t pageStart = 0;
static int8_t pageEnd = 0;

static char baseDir[40];

/*****************************************************************************/
// set RAM bank
#define setHighBank(bank) RAM_BANK = bank;

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
// loads to address
uint16_t lofad(char* filename, uint16_t address) {
    uint16_t ret = 0;
	cbm_k_setlfs(0,8,1);
	cbm_k_setnam(filename);
	ret = cbm_k_load(0,address)-address;
    cbm_k_close(0);
    return ret;
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
// check if file exists
bool checkFile(char* filename)
{
    int res = 0;
    int lfn = 2;
    int sad = 2;
    int i=0;
    char c = 0;
    char status[4];
    uint8_t err = 0;

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
                if (cbm_k_readst())
                    break;
                if (i < 4)
                {
                    status[i] = c;
                    i++;
                }
            }
            status[i] = 0;
            cbm_close(15);
            return status[0]=='0' && status[1]=='0';
        } 
    }
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
uint8_t getDirectory(struct DirCollection* dir, char* base, char* filter)
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

    if (base[0])
    {
        //strcpy(strDirLoad, "$//");
        //strcat(strDirLoad, base);
        //strcat(strDirLoad, "/:");
        changeDir(base);
    }
    if (filter[0]=='d')
        strcpy(strDirLoad, "$");
    else if (filter[0]=='p')
        strcpy(strDirLoad, "$:*=p");
    else
        strcpy(strDirLoad, "$");

    dir->numEntries = 0;
    setHighBank(2);
    len = bload(strDirLoad, 0xA000);
    //if (base[0])
    //    dump(0xA000, 20);

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

        if (filter[0]=='*')
        {
            isValid = (bool)strstr(name, ".thumb.abm");
            isValid |= (bool)strstr(name, ".meta.inf");
        }
        else if (filter[0]=='p' && !isPrg)
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

    if (base[0])
    {
        changeDir("..");
    }
    return index;    
}

/*****************************************************************************/
// get directory list
int getDirectoryList(char* base)
{
    uint8_t index = 0;
    uint8_t i = 0;
    uint8_t c = 0;
    uint8_t s = 0;
    char* szName = NULL;
    char prgName[30];
    char prgPath[100];    
    bool isDir = false;
    bool isPrg = false;

    // load directories first, add in alphabetical order
    getDirectory(&DirList, base, "dir");
    for (index=0; index < DirList.numEntries; index++)
    {
        for (i=0; i<=c; i++)
        {
            if (i==c || strcmp(DirList.arrEntries[index].name, arrDirectory[i].name) < 0)
            {
                memcpy(&arrDirectory[i+1], &arrDirectory[i], sizeof(struct DirEntry)*(c-i+2));
                memcpy(&arrDirectory[i], &DirList.arrEntries[index], sizeof(struct DirEntry));
                break;
            }
        }
        c++;
    }
/*    for (index=0; index < DirList.numEntries; index++)
    {
        memcpy(&arrDirectory[c], &DirList.arrEntries[index], sizeof(struct DirEntry));
        c++;
    }
*/
    // programs next - allow to sort stuff later.
    getDirectory(&DirList, base, "prg");
    s=c;
    for (index=0; index < DirList.numEntries; index++)
    {
        for (i=s; i<=c; i++)
        {
            if (i==c || strcmp(DirList.arrEntries[index].name, arrDirectory[i].name) < 0)
            {
                memcpy(&arrDirectory[i+1], &arrDirectory[i], sizeof(struct DirEntry)*(c-i+2));
                memcpy(&arrDirectory[i], &DirList.arrEntries[index], sizeof(struct DirEntry));
                break;
            }
        }
        c++;
    }

    numEntries = c;

    for (index=0; index < numEntries; index++)
    {
        arrDirectory[index].hasThumb = false;
        arrDirectory[index].hasMeta = false;
        if (arrDirectory[index].isDir)
        {
            // check if there is a PRG of identical name in that folder
            szName = arrDirectory[index].name;
            strcpy(prgName, szName);
            strcat(prgName, ".prg");

            strcpy(prgPath, "");
            strcat(prgPath, szName);
            strcat(prgPath, "/");
            strcat(prgPath, prgName);

 
            arrDirectory[index].hasThumb = false;
            arrDirectory[index].hasMeta = false;            
            arrDirectory[index].hasPrg = false;
            if (szName[0]=='.')
                continue;
//*
            arrDirectory[index].hasPrg = checkFile(prgPath);                
            arrDirectory[index].hasThumb = true;
            arrDirectory[index].hasMeta = true;            

/*
            getDirectory(&SubList, szName, "*");
            for (i=0; i<SubList.numEntries; i++)
            {
                //gotoxy(0,25+c);
                //printf(SubList.arrEntries[c++].name);
                if (strstr(SubList.arrEntries[i].name, prgName))
                {
                    arrDirectory[index].hasPrg = true;
                }
                else if (strstr(SubList.arrEntries[i].name, ".thumb.abm"))
                {
                    arrDirectory[index].hasThumb = true;
                }
                else if (strstr(SubList.arrEntries[i].name, "meta.inf"))
                {
                    arrDirectory[index].hasMeta = true;
                }
            } 
*/
 

        }
    }

    return numEntries;
}

/*****************************************************************************/
// get directory list
/*
int getDirectoryList_old(char* dir)
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
    char prgName[30];
    char prgPath[100];    
    bool isDir = false;
    bool isPrg = false;

    if (dir[0])
    {
        strcpy(strDirLoad, "$//");
        strcat(strDirLoad, dir);
        strcat(strDirLoad, "/:");
    }
    else
    {
        strcpy(strDirLoad, "$");
    }


    setHighBank(2);
    len = bload(strDirLoad, 0xA000);
    //dump(0xA000, len);

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

        isPrg = strstr(name, ".prg");
        isDir = type[0] == 'd';

        // only take files of type "dir" and "prg"
        if (name[1] != 0 && (isPrg || isDir) )
        {
            if (isPrg)
            {
                strcpy(type, "prg");
                arrDirectory[index].hasPrg = false;
                arrDirectory[index].isDir = false;
            }

            strcpy(arrDirectory[index].name, name);
            strcpy(arrDirectory[index].type, type);

            if (isDir)
            {
                arrDirectory[index].isDir = true;
                // check if there is a PRG of identical name in that folder
                strcpy(prgName, name);
                strcat(prgName, ".prg");

                strcpy(prgPath, "");
                strcat(prgPath, name);
                strcat(prgPath, "/");
                strcat(prgPath, prgName);

                arrDirectory[index].hasPrg = checkFile(prgPath);                
            }

            index++;
        }
    }    
    numEntries = index;

    return numEntries;    
}
*/

/*****************************************************************************/
// launch program
void launchPrg(char* name)
{
    while (kbhit()) cgetc();  // eat all keypresses before launching anything
    checkFile(""); // clear drive status

    // hide all sprites
    restoreScreenmode();

    // restore color and screen
    textcolor(6);
    bgcolor(6);
    clrscr();

    // print load command on screen"
    gotoxy(0,6);
    printf("load\"");
    printf(name);
    printf("\",8,1");

    // print run command
    gotoxy(0,11);
    printf("run");

    gotoxy(0,4);
    __asm__("lda #13");
    __asm__("jsr $fec3");
    __asm__("lda #13");
    __asm__("jsr $fec3");
}

/*****************************************************************************/
// launch program in diretory
void launch(char* dir, char* name)
{
    while (kbhit()) cgetc();  // eat all keypresses before launching anything
    checkFile(""); // clear drive status

    restoreScreenmode();

    // restore color and screen
    textcolor(6);
    bgcolor(6);
    clrscr();

    // print change dir command on screen"
    gotoxy(0,2);
    printf("dos\"cd");
/*    if (baseDir[0])
    {
        printf("//");
        printf(baseDir);
    }
*/    
    printf("/");
    printf(dir);
    printf("/:\"");

    // print load command on screen"
    gotoxy(0,6);
    printf("load\"");
    printf(name);
    printf("\",8,1");

    // print run command
    gotoxy(0,11);
    printf("run");

    // TRICK: put a number of "enter" keys into keyboard buffer
    gotoxy(0,0);
    __asm__("lda #13");
    __asm__("jsr $fec3");
    __asm__("lda #13");
    __asm__("jsr $fec3");
    __asm__("lda #13");
    __asm__("jsr $fec3");
}

/*****************************************************************************/
// check if file exists
bool checkFile_working(char* filename)
{
    int res = 0;
    int lfn = 2;
    int sad = 2;
    int i=0;
    char c = 0;
    char status[4];
    uint8_t err = 0;

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
                if (cbm_k_readst())
                    break;
                if (i < 4)
                {
                    status[i] = c;
                    i++;
                }
            }
            status[i] = 0;
            cbm_close(15);
            return status[0]=='0' && status[1]=='0';
        } 
    }
    return 0;
}

/*****************************************************************************/
// check if file exists
bool checkFile_mac(char* filename)
{
    int res = 0;
    int lfn = 1;
    int sad = 0;
    static char buffer[8];

    res = cbm_open(lfn,8,0,filename);
    res = cbm_k_chkin(lfn);
    if (res) return false;
    res = cbm_k_macptr(4, buffer);
    cbm_k_close(lfn);

    return res == 4;    
}

/*****************************************************************************/
// directory update
void updateDirList(int8_t selectedIndex)
{
    int8_t index=0;
    char strName[40];

    // hide all sprites
    hideAllSprites();

    if (numEntries <= maxEntries)
        pageEnd = pageStart + pageSize;
    if (pageEnd >= numEntries)
        pageEnd = numEntries;

    textcolor(0);
    bgcolor(15);

    gotoxy(27,2);
    if (pageStart > 0)
        screen_put_char('^');
    else
        screen_put_char(' ');

    gotoxy(27,23);
    if (pageEnd < numEntries && selectedIndex < maxEntries)
        screen_put_char(0x7e);
    else
        screen_put_char(' ');

    textcolor(1);
    bgcolor(6);
    for (index = pageStart; index < pageEnd; index++)
    {
        textcolor(index == selectedIndex ? 6 : 1);
        bgcolor(index == selectedIndex ? 1 : 6);

        gotoxy(4,index-pageStart+3);


        if (arrDirectory[index].hasPrg)
        {
            strcpy(strName, " ");
            //arrDirectory[index].hasPrg = true;
        }
        else if (arrDirectory[index].isPrg)
            strcpy(strName, " ");
        else if (arrDirectory[index].name[1] == '.')
            strcpy(strName, "<");
        else
            strcpy(strName, ">");
            
        strcat(strName, arrDirectory[index].name);
        if (arrDirectory[index].name[0] > '9' && arrDirectory[index].name[1] != '.')
            strName[1]+=32;
        printf("%-24s", strName);
    }
    index = 0;

}

/*****************************************************************************/
// show thumbnail (if available)
//  this loads a binary flat Bitmap file of 128x96 pixels 
//  and distributes it to 6 sprites (2 of 64x64 and 4 of 32x32)
void showThumbnail(int selectedIndex)
{
    uint16_t xOffset = 256+8;
    uint16_t yOffset = 16+4;
    int y = 0;
    int x = 0;
    int c = 0;
    unsigned char pixelValue = 0;
    char strThumbFile[40];

    if (!arrDirectory[selectedIndex].hasPrg && !arrDirectory[selectedIndex].isDir)
        return;

    // hide all sprites
    hideAllSprites();
   
    // don't show anything on the ".." (parent folder) entry
    if (arrDirectory[selectedIndex].name[0] == '.')
        return;

    strcpy(strThumbFile, "");
    strcat(strThumbFile, arrDirectory[selectedIndex].name);
    strcat(strThumbFile, "/.thumb.abm");

    if (!arrDirectory[selectedIndex].hasThumb)
        return;

    // load image
    if (!veraload(strThumbFile, 8, THUMBNAIL_BUFFER_ADDR))
    {
        arrDirectory[selectedIndex].hasThumb = false;
        //check_dos_error();
        //checkFile(strThumbFile);
        return;
    }

    split_thumbnail();

    //if (kbhit()) cgetc();

    createSprite(2, 64,64, xOffset,    yOffset, THUMBNAIL_BASE_ADDR, NULL);
    createSprite(4, 64,64, xOffset+64, yOffset, THUMBNAIL_BASE_ADDR+6144, NULL);
    createSprite(6, 64,64, xOffset+128,yOffset, THUMBNAIL_BASE_ADDR+6144*2, NULL);
    createSprite(8, 64,64, xOffset+192,yOffset, THUMBNAIL_BASE_ADDR+6144*3, NULL);
    createSprite(3, 64,32, xOffset,    yOffset+64, THUMBNAIL_BASE_ADDR+4096, NULL);
    createSprite(5, 64,32, xOffset+64, yOffset+64, THUMBNAIL_BASE_ADDR+6144+4096, NULL);
    createSprite(7, 64,32, xOffset+128,yOffset+64, THUMBNAIL_BASE_ADDR+6144*2+4096, NULL);
    createSprite(9, 64,32, xOffset+192,yOffset+64, THUMBNAIL_BASE_ADDR+6144*3+4096, NULL);

}

/*****************************************************************************/
// show meta information (if available)
//  this loads a text file. 1St line is the game name, 2nd line is the Authors name
//  Following is the description text
void showMeta(int selectedIndex)
{
    char strMetaFile[40];
    uint16_t len = 0;
    char* szMeta = (char*)0xA000;
    char* szTitle = 0;
    char* szAuthor = 0;
    char* szDesc = 0;
    char c = 0;

    uint8_t x = 0;
    uint8_t y = 0;
    uint16_t i = 0;
    bool boPrint = false;

    strcpy(strMetaFile, "");
    strcat(strMetaFile, arrDirectory[selectedIndex].name);
    strcat(strMetaFile, "/.meta.inf");

    // load meta information
    setHighBank(2);
    // background for meta information
    bgcolor(6);
    textcolor(1);
    for (i=0; i<9; i++)
    {
        gotoxy(32,16+i);
        printf("                                      ");
    }

    if (!arrDirectory[selectedIndex].hasMeta)
        return;

    if (!arrDirectory[selectedIndex].hasPrg && !arrDirectory[selectedIndex].isDir)
    {
        return;
    }
    // don't show anything on the ".." (parent folder) entry
    if (arrDirectory[selectedIndex].name[0] == '.')
    {
        gotoxy(32,16);
        printf("Navigate to parent directory.");
        return;
    }

    len = bload(strMetaFile, 0xA000);    
    if (!len)
    {
        arrDirectory[selectedIndex].hasMeta = false;
        //check_dos_error();
        //checkFile(strMetaFile);
        return;
    }
    // fix upper lower-case
    szMeta[len] = 0;
    for (i=0; i<len; i++)
    {
        c = szMeta[i];
        if (c >= 64 && c<64+27)
            szMeta[i]+=32;
        else if (c > 96 && c<96+27)
            szMeta[i]-=32;
    }

    // get title, author and description strings
    for (i=0; i<len; i++)
    {
        if (szMeta[i] == ':')
        {
            i++;
            if (szMeta[i] == ' ')
                i++;
            if (!szTitle)
                szTitle = szMeta + i;
            else if (!szAuthor)
                szAuthor = szMeta + i;
        }
        if (szMeta[i] == '\r')
        {
            if (szTitle && !szAuthor)
                szMeta[i] = 0;
            if (szTitle && szAuthor && !szDesc)
            {
                szMeta[i] = 0;
                szDesc = szMeta + i+1;
            }
        }
    }
    // show information
    gotoxy(32,16);
    printf(szTitle);
    textcolor(15);
    gotoxy(32,17);
    printf("by: %s", szAuthor);
    textcolor(1);

    if (kbhit())
        return;

    // show description with word wrap
    len -= (szDesc - szMeta);
    szDesc[len] = 0;
    x = 32;
    y = 19;
    boPrint = true;
    for (i=0; i<len; i++)
    {
        if (boPrint)
        {
            gotoxy(x,y);
            cbm_k_chrout(szDesc[i]);
            x++;
            // word wrap (crude, but kind of working)
            if (szDesc[i] == ' ' && x > 60)
            {
                y++;
                x=32;
            }
        }
    }    
    textcolor(1);
    bgcolor(6);    

    //if (kbhit()) cgetc();

}

/*****************************************************************************/
// directory navigation
int navigate()
{
    uint16_t joy_matrix=0;
    uint16_t last_matrix=0;
    char key=0;
    int index=arrDirectory[0].name[0] == '.' ? 1 : 0;
    textcolor(2);

    if (kbhit())
        cgetc(); 

    while (true)
    {
        updateDirList(index);
        if (!kbhit())
            showMeta(index);
        if (!kbhit())
            showThumbnail(index);
        // check keyboard and joystick
        while (!kbhit())
        { 
    //         joy_matrix = joy_read(JOY_1);
    // gotoxy(0,0);
    // printf("%02X, %d",joy_matrix, joy_count());
    //         if ((joy_matrix & 0xF0FF) != 0)
    //         {
    //             if (JOY_RIGHT(joy_matrix) || JOY_START(joy_matrix) || JOY_BTN_1(joy_matrix) || JOY_BTN_2(joy_matrix) || JOY_BTN_3(joy_matrix) || JOY_BTN_4(joy_matrix))
    //             {
    //                 __asm__("lda #$0D");
    //                 __asm__("jsr $fec3");            
    //             }
    //             if (JOY_LEFT(joy_matrix))
    //             {
    //                 __asm__("lda #$2E");
    //                 __asm__("jsr $fec3");            
    //             }
    //             else if (JOY_UP(joy_matrix))
    //             {
    //                 __asm__("lda #$91");
    //                 __asm__("jsr $fec3");            
    //             }
    //             else if (JOY_DOWN(joy_matrix))
    //             {
    //                 __asm__("lda #$11");
    //                 __asm__("jsr $fec3");            
    //             }
    //             // wait for change = release of last button.
    //             last_matrix = joy_matrix;
    //             do
    //             {
    //                 joy_matrix = joy_read(JOY_2); 
    //             }
    //             while (joy_matrix == last_matrix);
    //        }
        }
        key = cgetc();

        while (kbhit()) cgetc();         

        //gotoxy(0,1);
        //printf("%02X", key);
        switch (key)
        {
            case 0x1B: // esc
                return -1;
            break;
            case 0x13: // home
                index = 0;
                pageStart = 0;
            break;
            case 0x82: // page up
                if (index <=0) break;
                index -= pageSize-1;
                pageStart -= pageSize-1;
                if (index < 0)
                    index = 0;
            break;
            case 0x91: // up
                if (index <=0) break;
                index--;
                if (index < pageStart)
                    pageStart = index;
            break;
            case 0x11: // down
                if (index >= numEntries-1) break;
                index++;
            break;
            case 0x02: // page down
                index += pageSize-1;
                if (index >= numEntries-1)
                    index = numEntries-1;
                pageStart += pageSize-1;
            break;
            case 0x2E: // dot = directory up
                index = 0;
                pageStart = 0;
                return index;
            break;
            case 0x04: // end
                index = numEntries-1;
            break;
            case 0x0D: // enter key
                return index;
            break;
        }

        // check pageing
        if (pageStart >= numEntries)
            pageStart = numEntries-pageSize;
        pageEnd = pageStart + pageSize;
        if (pageEnd > numEntries)
        {
            pageEnd = numEntries;
            pageStart = pageEnd - pageSize;
        }
        if (index >= pageEnd)
            pageStart = index - pageSize+1;
        if (pageStart < 0)
            pageStart = 0;
        pageEnd = pageStart + pageSize;
    }

}

/*****************************************************************************/
// screen layout and decorations
void drawLayout()
{
    int i=0;

    // frame for directory list
    bgcolor(6);
    textcolor(15);
    for (i=0; i<=21; i++)
    {
        gotoxy(3,2+i);
        printf("\xb6                        \xb4");
    }
    gotoxy(4,2);
    bgcolor(15);
    textcolor(6);

    strcpy(baseDir, getCurrentDirectory());
    if (baseDir[0] > '9' && baseDir[1] != '.')
        baseDir[0]+=32;


    if (baseDir[0]=='/')
        printf(" %-20s   ", baseDir);
    else
        printf(" ./%-20s", baseDir);

    textcolor(0);
    bgcolor(15);

    gotoxy(4,23);
    printf(" use arrow keys + enter ");

    // background for thumbnail
    bgcolor(0);
    textcolor(1);
    for (i=0; i<=12; i++)
    {
        gotoxy(32,2+i);
        printf("                                  ");
    }

    bgcolor(6);
    gotoxy(6,3);
    printf("loading...");

    textcolor(15);
    gotoxy(4,25);
    printf("Launcher (c) by AndyMt");
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


/*****************************************************************************/
// main program
int main(int argc, char *argv[])
{
    char key=argc;
    int index=0;
    char prgName[40];
    char prgPath[80];
    char* szName=NULL;

    baseDir[0]=0;
    strcpy(baseDir, "/");

    SetupScreenMode();

    joy_install(cx16_std_joy);

    // empty keyboard buffer first
    while (kbhit())
    { cgetc(); }

    // prepare directory buffers
    setupDirectory(&DirList, 50);
    setupDirectory(&SubList, 10);

repeat:
    clrscr();
    drawLayout();
    getDirectoryList("");    
    index = navigate();
    if (index == -1)
    {
        checkFile(""); // clear drive status
        restoreScreenmode();
        return 0;
    }
    
    strcpy(prgName, arrDirectory[index].name);

    if (arrDirectory[index].type[0] == 'd') // selected a directory
    {
        strcat(prgName, ".prg");

        strcpy(prgPath, "");
        strcat(prgPath, arrDirectory[index].name);
        strcat(prgPath, "/");
        strcat(prgPath, prgName);

        if (checkFile(prgPath)) // program file exists => launch it
        {
            launch(arrDirectory[index].name, prgName);
            return 0;
        }
        else // change directory
        {
            changeDir(arrDirectory[index].name);
            goto repeat;
        }
    }
    else // launch prg in current directory
    {
        launchPrg(prgName); 
    }

    return 0;
}
