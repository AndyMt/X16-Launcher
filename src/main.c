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
#include "dirtools.h"
#include "inifile.h"

/*****************************************************************************/

extern uint8_t res1;
extern uint8_t res2;
extern uint8_t res3;
extern uint16_t VarTab;
extern char* Name;

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

extern void LaunchPrg();

void showTextWrapped(char* strText, uint8_t x, uint8_t y, uint8_t w, uint8_t h);

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
        memcpy(&arrDirectory[c], &DirList.arrEntries[index], sizeof(struct DirEntry));
        c++;
    }

    // programs next - allow to sort stuff later.
    getDirectory(&DirList, base, "prg");
    for (index=0; index < DirList.numEntries; index++)
    {
        memcpy(&arrDirectory[c], &DirList.arrEntries[index], sizeof(struct DirEntry));
        c++;
    }    
    numEntries = c;

    for (index=0; index < numEntries; index++)
    {
        arrDirectory[index].hasThumb = true;
        arrDirectory[index].hasMeta = true;
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

            arrDirectory[index].hasPrg = false;
            if (szName[0]=='.')
                continue;

            arrDirectory[index].hasPrg = checkFile(prgPath);                

        }
    }

    return numEntries;
}


/*****************************************************************************/
// get directory list
int getDirectoryListSorted(char* base)
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
        if (strcmp(DirList.arrEntries[index].name, "launcher")==0)
            continue;

        for (i=0; i<=c; i++)
        {
            if (i==c || strcmp(DirList.arrEntries[index].name, arrDirectory[i].name) < 0)
            {
                memmove(&arrDirectory[i+1], &arrDirectory[i], sizeof(struct DirEntry)*(c-i+2));
                memcpy(&arrDirectory[i], &DirList.arrEntries[index], sizeof(struct DirEntry));
                break;
            }
        }
        c++;
    }

    // programs next - allow to sort stuff later.
    getDirectory(&DirList, base, "prg");
    s=c;
    for (index=0; index < DirList.numEntries; index++)
    {
        for (i=s; i<=c; i++)
        {
            if (i==c || strcmp(DirList.arrEntries[index].name, arrDirectory[i].name) < 0)
            {
                memmove(&arrDirectory[i+1], &arrDirectory[i], sizeof(struct DirEntry)*(c-i+2));
                memcpy(&arrDirectory[i], &DirList.arrEntries[index], sizeof(struct DirEntry));
                break;
            }
        }
        c++;
    }

    numEntries = c;

    for (index=0; index < numEntries; index++)
    {
        arrDirectory[index].hasThumb = true;
        arrDirectory[index].hasMeta = true;
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

 
            arrDirectory[index].hasPrg = false;
            if (szName[0]=='.')
                continue;
//*
            arrDirectory[index].hasPrg = checkFile(prgPath);                

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
    textcolor(1);
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
    textcolor(1);
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

    gotoxy(29,2);
    if (pageStart > 0)
        screen_put_char('^');
    else
        screen_put_char(' ');

    gotoxy(29,23);
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
            //strcpy(strName, "\x7F");
            strcpy(strName, " ");
            //arrDirectory[index].hasPrg = true;
        }
        else if (arrDirectory[index].isPrg)
            strcpy(strName, " ");
        else if (arrDirectory[index].name[1] == '.')
            strcpy(strName, "\x7C");
        else
            strcpy(strName, "\x7D");
            
        strcat(strName, arrDirectory[index].name);
        if (arrDirectory[index].name[0] > '9' && arrDirectory[index].name[1] != '.')
            strName[1]+=32;
        printf("%-26s", strName);
    }
    index = 0;

}

/*****************************************************************************/
char* getThumbnailFileName(int selectedIndex, int subIndex)
{
    static char strThumbFile[80];

    if (isLocalMode)
    {
        strcpy(strThumbFile, "");
        strcat(strThumbFile, arrDirectory[selectedIndex].name);
        strcat(strThumbFile, "/.thumb");
    }
    else
    {
        strcpy(strThumbFile, launcherDir);
        strcat(strThumbFile, "/");
        strcat(strThumbFile, arrDirectory[selectedIndex].name);
    }
    if (subIndex)
    {
        strThumbFile[strlen(strThumbFile)+1] = 0;
        strThumbFile[strlen(strThumbFile)] = '0'+subIndex;
    }
    strcat(strThumbFile, ".abm");    

    if (subIndex)
    {
        if (checkFile(strThumbFile))
        {
            //check_dos_error();
            return strThumbFile;
        }
        else
        {
            return NULL;
        }
    }
    return strThumbFile;
}

/*****************************************************************************/
char* getDefaultThumbnailFileName(int selectedIndex)
{
    static char strThumbFile[80];
    if (isLocalMode)
        return NULL;

    strcpy(strThumbFile, launcherDir);
    strcat(strThumbFile, "/");
    if (arrDirectory[selectedIndex].isDir && !arrDirectory[selectedIndex].hasPrg)
        strcat(strThumbFile, ".folder.abm");
    else if (arrDirectory[selectedIndex].isPrg)
        strcat(strThumbFile, ".program.abm");
    else
        strcat(strThumbFile, ".program.abm");
        //return NULL;
    return strThumbFile;
}

/*****************************************************************************/
// show thumbnail (if available)
//  this loads a binary flat Bitmap file of 128x96 pixels 
//  and distributes it to 6 sprites (2 of 64x64 and 4 of 32x32)
void showThumbnail(int selectedIndex, int subIndex)
{
    uint16_t xOffset = 256+8;
    uint16_t yOffset = 16+4;
    int y = 0;
    int x = 0;
    int c = 0;
    struct DirEntry* pEntry = &arrDirectory[selectedIndex];
    unsigned char pixelValue = 0;
    char* strThumbFile=NULL;

    if (!pEntry->hasPrg && !pEntry->isDir && !pEntry->isPrg)
        return;

    // hide all sprites
    hideAllSprites();
   
    // don't show anything on the ".." (parent folder) entry
    if (pEntry->name[0] == '.')
        return;

    if (pEntry->hasThumb || isLocalMode)
        strThumbFile = getThumbnailFileName(selectedIndex, subIndex);
    else
        strThumbFile = getDefaultThumbnailFileName(selectedIndex);
    if (!strThumbFile)
        return;

    // load image
    if (!veraload(strThumbFile, 8, THUMBNAIL_BUFFER_ADDR))
    {
        strThumbFile = getDefaultThumbnailFileName(selectedIndex);

        if (!strThumbFile || !veraload(strThumbFile, 8, THUMBNAIL_BUFFER_ADDR))
        {
            pEntry->hasThumb = false;
            return;
        }
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

/*****************************************************************************/
char* getMetaFileName(int selectedIndex)
{
    static char strMetaFile[60];

    if (isLocalMode)
    {
        strcpy(strMetaFile, "");
        strcat(strMetaFile, arrDirectory[selectedIndex].name);
        strcat(strMetaFile, "/.meta");
    }
    else
    {
        strcpy(strMetaFile, launcherDir);
        strcat(strMetaFile, "/");
        strcat(strMetaFile, arrDirectory[selectedIndex].name);
    }
    strcat(strMetaFile, ".inf");    

    return strMetaFile;
}

/*****************************************************************************/
// show meta information (if available)
//  this loads a text file. 1St line is the game name, 2nd line is the Authors name
//  Following is the description text
void showMeta(int selectedIndex)
{
    char* strMetaFile;
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

    strMetaFile = getMetaFileName(selectedIndex);

    // load meta information
    setHighBank(2);
    // background for meta information
    bgcolor(6);
    textcolor(1);
    for (i=0; i<10; i++)
    {
        gotoxy(32,16+i);
        printf("                                      ");
    }

    if (!arrDirectory[selectedIndex].hasMeta && isLocalMode)
        return;

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

    //showTextWrapped(szDesc, 32, 19, 35, 6);

     
    x = 32;
    y = 19;
    boPrint = true;
    for (i=0; i<len; i++)
    {
        if (boPrint)
        {
            gotoxy(x,y);
            if (szDesc[i] == '\r')
                continue;
            if (szDesc[i] == '\n')
            {
                y++;
                x=32;
                continue;
            }
            cbm_k_chrout(szDesc[i]);
            x++;
            // word wrap (crude, but kind of working)
            if (szDesc[i] == ' ' && x > 60)
            {
                y++;
                if (y>25)
                    break;
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
    static uint8_t tick = 0;
    char key = 0;
    int index = arrDirectory[0].name[0] == '.' ? (numEntries>1 ? 1 : 0) : 0;
    int i = 0;
    int subIndex = 0;
    textcolor(2);

    if (kbhit())
        cgetc(); 

    while (true)
    {
        updateDirList(index);
        if (!kbhit())
            showMeta(index);
        if (!kbhit())
            showThumbnail(index, 0);

        subIndex = 0;
repeat:
        tick = 0;
        // check keyboard and joystick
        while (!kbhit())
        { 
            vsync();
            tick++;

            // handle automatic thumbnail slideshow
            if (tick > 180 && arrDirectory[index].hasThumb)
            {
                tick = 0;
                if (getThumbnailFileName(index,subIndex+1))
                {
                    subIndex++;
                    showThumbnail(index, subIndex);
                    goto repeat;
                } 
                else if (subIndex > 0)
                {
                    subIndex = 0;
                    showThumbnail(index, subIndex);
                    goto repeat;
                }
            }
            joy_matrix = joy_read(JOY_2);
    //gotoxy(0,0);
    //printf("%02X, %d",joy_matrix, joy_count());
            if ((joy_matrix & 0xFFFF) != 0xFF00)
            {
                if (JOY_BTN_1(joy_matrix) || JOY_BTN_2(joy_matrix) ||JOY_BTN_4(joy_matrix))
                {
                    __asm__("lda #$0D");
                    __asm__("jsr $fec3");            
                }
                if (JOY_LEFT(joy_matrix))
                {
                    __asm__("lda #$2E");
                    __asm__("jsr $fec3");            
                }
                else if (JOY_UP(joy_matrix))
                {
                    __asm__("lda #$91");
                    __asm__("jsr $fec3");            
                }
                else if (JOY_DOWN(joy_matrix))
                {
                    __asm__("lda #$11");
                    __asm__("jsr $fec3");            
                }
                
                // wait for change = release of last button.
                last_matrix = joy_matrix;
                i=0;
                do
                {
                    joy_matrix = joy_read(JOY_2); 
                    vsync();
                    tick++;
                    i++;
                }
                while (joy_matrix == last_matrix && i<10);
                
           }
        }
        key = cgetc();

        while (kbhit()) cgetc();         

        // letter input => search for nearest index
        if (key>='a' && key <='z')
        {
            for (i=0; i<numEntries; i++)
            {
                if (arrDirectory[i].name[0] == key)
                {
                    if (arrDirectory[index].name[0]==key && i > index || arrDirectory[index].name[0]!=key)
                    {
                        index = i;
                        break;
                    }
                }
            }
        }
        //gotoxy(0,1);
        //printf("%02X", key);
        switch (key)
        {
            case 0x1B: // esc
                return -1;
            break;
            case 0x9D: // left
                if (subIndex > 0) subIndex--;
                showThumbnail(index, subIndex);
                goto repeat;
            break;
            case 0x1D: // right
                if (getThumbnailFileName(index,subIndex+1))
                {
                    subIndex++;
                    showThumbnail(index, subIndex);
                    goto repeat;
                }
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
                if (arrDirectory[0].name[0]=='.')
                {
                    index = 0;
                    pageStart = 0;
                    pageEnd = pageStart + pageSize;
                    return index;
                }
                else
                    goto repeat;
            break;
            case 0x04: // end
                index = numEntries-1;
            break;
            case 0x0D: // enter key
                pageStart = 0;
                pageEnd = pageStart + pageSize;
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
        printf("\xb6                          \xb4");
    }
    gotoxy(4,2);
    bgcolor(15);
    textcolor(6);

    strcpy(baseDir, getCurrentDirectory());
    if (baseDir[0] > '9' && baseDir[1] != '.')
        baseDir[0]+=32;


    if (baseDir[0]=='/')
        printf(" %-25s", baseDir);
    else
        printf(" ./%-23s", baseDir);

    textcolor(0);
    bgcolor(15);

    gotoxy(4,23);
    printf(" use arrow keys + enter   ");

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
// show intro screen
void showTextWrapped(char* strText, uint8_t x, uint8_t y, uint8_t w, uint8_t h)
{
    int i=0;
    int p=0;
    int len=strlen(strText);
    uint8_t t = y;
    char c=0;
    char* szWord=strText;

    gotoxy(x, y);

    for (i=0; i<=len; i++)
    {
        c=strText[i];
        if (c==' ' || c== '\r' || i == len)
        {
            strText[i] = 0;
            if (p+strlen(strText) > w)
            {
                p=0;
                y++;
                gotoxy(x, y);
            }
            gotoxy(x+p, y);
            printf(szWord);
            p+=strlen(szWord)+1; // include space
            if (c=='\r')
            {
                i++;
                p=0;
                y++;
                gotoxy(x, y);
            }
            else
                printf(" ");
                if (y > t+h)
                    return;
            szWord = strText+i+1;
        }
    }

}

/*****************************************************************************/
// show intro screen
void showIntroScreen()
{
    uint8_t y;
    static char* szIntro = 
        "This launcher let's you navigate through directories and start programs.\r\n"
        "For a selection of games and programs a thumbnail and short description is shown. "
        "For all others default icons are shown.\r\n\r\n"
        "Navigation by keybard:\r\n"
        "- up/down:    scroll through directory list\r\n"
        "- left/right: cycle through thumbnails (if multiple)\r\n"
        "- enter:      select directory or start program\r\n"
        "- any letter: jump directly in the list\r\n"
        "- esc:        exit launcher\r\n\r\n"
        "Navigation by joystick/controller:\r\n"
        "- up/down:    scroll through directory list\r\n"
        "- left/right: cycle through thumbnails (if multiple)\r\n"
        "- any button: select directory or start program\r\n"
        ;

    clrscr();
    while (kbhit())
    { cgetc(); }

    bgcolor(15);
    textcolor(6);

    gotoxy(7,2);
    printf(" %-54s ", "Launcher (c) by AndyMt: Introduction");

    bgcolor(6);
    textcolor(15);

    for (y=2; y<24; y++)
    {
        gotoxy(6, y);
        cbm_k_chrout('\xb6');
        gotoxy(63, y);
        cbm_k_chrout('\xb4');
    }
    bgcolor(15);
    textcolor(6);
    gotoxy(7,23);
    printf(" %-54s ", "This message is only shown once. Press any key...");

    bgcolor(6);
    textcolor(1);
    showTextWrapped(szIntro, 8,4,53,18);

}

/*****************************************************************************/
void manageInfoscreen()
{
    struct ini_section *sections = NULL;
    char* szValue=NULL;

    //show intro screen?
    changeDir(launcherDir);
    sections = read_ini(".launcher.tmp");

    if (!sections) // does not exist? show intro
    {
        showIntroScreen();
        sections = create_ini_section(sections, "State");
        set_ini_property(sections, "skipintro", "1");
        save_ini(".launcher.tmp", sections);

        while (!kbhit()) {} while (kbhit()) { cgetc(); }

    }
    else // otherwise store ini to skip it next time
    {
        szValue = get_ini_property(sections, "skipintro")->value;
        if (szValue[0]=='0' || szValue[0]=='n')
        {
            showIntroScreen();
            set_ini_property(sections, "skipintro", "1");
            save_ini(".launcher.tmp", sections);
            while (!kbhit()) {} while (kbhit()) { cgetc(); }
        }
    }

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
    char* szValue=NULL;
    struct ini_section *sections = NULL;
    struct ini_section *curr_section = sections;

    baseDir[0]=0;
    strcpy(baseDir, "/");
    strcpy(startDir, "");
    strcpy(launcherDir, "");
    isLocalMode = true;

    while (kbhit())
    { cgetc(); }
    
    sections = read_ini("launcher.ini");
    if (sections) // ini file is there, get values
    {
        szValue = get_ini_property(sections, "localmode")->value;
        isLocalMode = szValue[0]=='1' || szValue[0]=='y';
        szValue = get_ini_property(sections, "launcherDir")->value;
        if (szValue)
        {
            strcpy(launcherDir, szValue);
        }
        szValue = get_ini_property(sections, "startDir")->value;
        if (szValue)
        {
            strcpy(startDir, szValue);
        }
    }
    curr_section = sections;

    SetupScreenMode();

    joy_install(cx16_std_joy);

    manageInfoscreen();

    if (startDir[0])
        changeDir(startDir);

    // empty keyboard buffer first
    while (kbhit())
    { cgetc(); }

    // prepare directory buffers
    setupDirectory(&DirList, 50);
    setupDirectory(&SubList, 10);

repeat:
    clrscr();
    drawLayout();
    getDirectoryListSorted("");    
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
       return 0;
    }

    return 0;
}
