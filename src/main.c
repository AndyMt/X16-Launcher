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
#include <vload.h>
#include "graphics.h"
#include "util.h"

extern void LaunchPrg();
extern uint8_t res1;
extern uint8_t res2;
extern uint8_t res3;
extern uint16_t VarTab;
extern char* Name;

struct DirEntry
{
    char name[32];
    char type[8];
    bool hasPrg;
};

static struct DirEntry arrEntries[50];
static int8_t numEntries = 0;
static int8_t maxEntries = 50;

static int8_t pageSize = 20;
static int8_t pageStart = 0;
static int8_t pageEnd = 0;

static char baseDir[80];
static char buffer[512];



/*****************************************************************************/
// set RAM bank#define RAM_BANK        (*(unsigned char *)0x00)
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
uint16_t load(char* filename, uint16_t address) {
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
        y = index / 16 + 2;
        if (x > 22)
            x++;
        gotoxy(x,y);
        printf("%02X", b);

        x = index % 16 + 50;
        y = index / 16 + 2;
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
// list directory
int listDirectory(char* dir)
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

        // only take files of type "dir"
        if (type[0] == 'd' && name[1] != 0)
        {
            strcpy(arrEntries[index].name, name);
            strcpy(arrEntries[index].type, type);

            // check if there is a PRG of identical name in that folder
            strcpy(prgName, name);
            strcat(prgName, ".prg");

            strcpy(prgPath, "");
            strcat(prgPath, name);
            strcat(prgPath, "/");
            strcat(prgPath, prgName);

            arrEntries[index].hasPrg = checkFile(prgPath);                

            index++;
        }
    }    
    numEntries = index;

    return numEntries;    
}

/*****************************************************************************/
// launch program
void launch(char* dir, char* name)
{
    int i=0;

    // hide all sprites
    videomode(VIDEOMODE_80x30);
    for (i = 0; i < 10; i++)
    {
        hideSprite(i);
    }    
    // restore color and screen
    textcolor(6);
    bgcolor(6);
    clrscr();

    // print change dir command on screen"
    gotoxy(0,2);
    printf("dos\"cd");
    if (baseDir[0])
    {
        printf("//");
        printf(baseDir);
    }
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

    // TRICK: put 3x "enter" key into keyboard buffer
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


    if (numEntries <= maxEntries)
        pageEnd = pageStart + pageSize;
    if (pageEnd >= numEntries)
        pageEnd = numEntries;

    textcolor(0);
    bgcolor(15);

    gotoxy(4,2);
    if (pageStart > 0)
    {
        gotoxy(27,2);
        printf("^");
    }
    gotoxy(27,23);
    if (pageEnd < numEntries && selectedIndex < maxEntries)
        printf("\xB2");
    else
        printf(" ");

    textcolor(1);
    bgcolor(6);
    for (index = pageStart; index < pageEnd; index++)
    {
        textcolor(index == selectedIndex ? 6 : 1);
        bgcolor(index == selectedIndex ? 1 : 6);

        gotoxy(4,index-pageStart+3);


        if (arrEntries[index].hasPrg)
        {
            strcpy(strName, " ");
            arrEntries[index].hasPrg = true;
        }
        else if (arrEntries[index].name[1] == '.')
            strcpy(strName, "<");
        else
            strcpy(strName, ">");
            
        strcat(strName, arrEntries[index].name);
        if (arrEntries[index].name[0] > '9' && arrEntries[index].name[1] != '.')
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

    // hide all sprites
    for (c = 0; c < 10; c++)
    {
        hideSprite(c);
    }    
    // don't show anything on the ".." (parent folder) entry
    if (arrEntries[selectedIndex].name[0] == '.')
        return;

    strcpy(strThumbFile, "");
    strcat(strThumbFile, arrEntries[selectedIndex].name);
    strcat(strThumbFile, "/.thumb.abm");

    // load image
    if (!vload(strThumbFile, 8, THUMBNAIL_BUFFER_ADDR))
    {
        checkFile(strThumbFile);
        return;
    }

    split_thumbnail();

    createSprite(2, 64,64, xOffset,    yOffset, THUMBNAIL_BASE_ADDR, NULL);
    createSprite(4, 64,64, xOffset+64, yOffset, THUMBNAIL_BASE_ADDR+6144, NULL);
    createSprite(6, 64,64, xOffset+128,yOffset, THUMBNAIL_BASE_ADDR+6144*2, NULL);
    createSprite(8, 64,64, xOffset+192,yOffset, THUMBNAIL_BASE_ADDR+6144*3, NULL);
    createSprite(3, 64,32, xOffset,    yOffset+64, THUMBNAIL_BASE_ADDR+4096, NULL);
    createSprite(5, 64,32, xOffset+64, yOffset+64, THUMBNAIL_BASE_ADDR+6144+4096, NULL);
    createSprite(7, 64,32, xOffset+128,yOffset+64, THUMBNAIL_BASE_ADDR+6144*2+4096, NULL);
    createSprite(9, 64,32, xOffset+192,yOffset+64, THUMBNAIL_BASE_ADDR+6144*3+4096, NULL);
}

void showThumbnail_test(int selectedIndex)
{
    int xOffset = 256+8;
    int yOffset = 16+4;
    int y = 0;
    int x = 0;
    int c = 0;
    unsigned char pixelValue = 0;
    uint16_t addr = THUMBNAIL_BASE_ADDR;
    uint16_t offset = THUMBNAIL_BUFFER_ADDR;
    char strThumbFile[40];

    // hide all sprites
    for (c = 0; c < 10; c++)
    {
        hideSprite(c);
    }    
    // don't show anything on the ".." (parent folder) entry
    if (arrEntries[selectedIndex].name[0] == '.')
        return;

    strcpy(strThumbFile, "");
    strcat(strThumbFile, arrEntries[selectedIndex].name);
    strcat(strThumbFile, "/.thumb.abm");

    // load image
    if (!vload(strThumbFile, 8, THUMBNAIL_BASE_ADDR))
    {
        //checkFile(strThumbFile);
        return;
    }

    createSprite(2, 64,64, xOffset,    yOffset, THUMBNAIL_BASE_ADDR, NULL);
    createSprite(4, 64,64, xOffset+64, yOffset, THUMBNAIL_BASE_ADDR+6144, NULL);
    createSprite(6, 64,64, xOffset+128,yOffset, THUMBNAIL_BASE_ADDR+6144*2, NULL);
    createSprite(8, 64,64, xOffset+192,yOffset, THUMBNAIL_BASE_ADDR+6144*3, NULL);
    createSprite(3, 64,32, xOffset,    yOffset+64, THUMBNAIL_BASE_ADDR+4096, NULL);
    createSprite(5, 64,32, xOffset+64, yOffset+64, THUMBNAIL_BASE_ADDR+6144*1+4096, NULL);
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
    strcat(strMetaFile, arrEntries[selectedIndex].name);
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

    // don't show anything on the ".." (parent folder) entry
    if (arrEntries[selectedIndex].name[0] == '.')
    {
        gotoxy(32,16);
        printf("Navigate to parent directory.");
        return;
    }

    len = bload(strMetaFile, 0xA000);    
    if (!len)
    {
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
}

/*****************************************************************************/
// directory navigation
int navigate()
{
    char key=0;
    int index=arrEntries[0].name[0] == '.' ? 1 : 0;
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
        while (!kbhit())
        { }
        key = cgetc();
        //gotoxy(0,0);
        //printf("%02X", key);
        switch (key)
        {
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
            case 0x04: // end
                index = numEntries-1;
            break;
            case 0x51:
                screen_set_charset(3);
                videomode(VIDEOMODE_80x30);
                return -1;
            case 0x0D: // enter key
                screen_set_charset(3);
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
    printf(" Launch:                ");

    textcolor(0);
    bgcolor(15);

    gotoxy(4,23);
    printf(" use arrow keys + enter ");
  //printf(" Launch:                ");

    // background for thumbnail
    bgcolor(0);
    textcolor(1);
    for (i=0; i<=12; i++)
    {
        gotoxy(32,2+i);
        printf("                                  ");
    }

    bgcolor(6);
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
    res = cbm_open(lfn,8,sad,command);
    cbm_k_close(lfn);

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
    baseDir[0]=0;
    //strcpy(baseDir,"games/");

/*     clrscr();
    checkFile("launcher.prg");
    checkFile("err.prg");
    checkFile("launcher.prg");
    checkFile("err.prg");

    while(!kbhit())     {}
return 0; */

    SetupScreenMode();

    // empty keyboard buffer first
    while (kbhit())
    { cgetc(); }

repeat:
    clrscr();
    screen_set_charset(3);
    drawLayout();
    listDirectory(baseDir);    
    index = navigate();
    if (index == -1)
        return 0;
    strcpy(prgName, arrEntries[index].name);
    strcat(prgName, ".prg");

    strcpy(prgPath, "");
    strcat(prgPath, arrEntries[index].name);
    strcat(prgPath, "/");
    strcat(prgPath, prgName);

    if (checkFile(prgPath)) // program file exists => launch it
    {
        launch(arrEntries[index].name, prgName);
        return 0;
    }
    else // change directory
    {
        changeDir(arrEntries[index].name);
        goto repeat;
    }
    return 0;
}
