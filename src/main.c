#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <cx16.h>
#include <conio.h>
#include <cbm.h>
#include <errno.h>
#include <joystick.h>
#include <vload.h>
#include "globals.h"
#include "graphics.h"
#include "util.h"
#include "dirtools.h"
#include "inifile.h"
#include "launch.h"
#include "intro.h"

//----------------------------------------------------------------------------
void drawLayout();

//----------------------------------------------------------------------------
// get sorted directory list
int getDirectoryListSorted()
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
    getDirectory(&DirList, "dir");
    for (index=0; index < DirList.numEntries; index++)
    {
        szName = DirList.arrEntries[index].name;
        if (strstr(thumbDir, DirList.arrEntries[index].name))
            continue;
        if (strstr(metaDir, DirList.arrEntries[index].name))
            continue;

        if (strstr(startDir, baseDir) && szName[0]=='.')
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

    // programs next, add in alphabetical order
    getDirectory(&DirList, "prg");
    s=c;
    for (index=0; index < DirList.numEntries; index++)
    {
        szName = DirList.arrEntries[index].name;
        if (strcmp(szName, "launcher.prg")==0)
            continue;

        for (i=s; i<=c; i++)
        {
            if (i==c || strcmp(szName, arrDirectory[i].name) < 0)
            {
                memmove(&arrDirectory[i+1], &arrDirectory[i], sizeof(struct DirEntry)*(c-i+2));
                memcpy(&arrDirectory[i], &DirList.arrEntries[index], sizeof(struct DirEntry));
                break;
            }
        }
        c++;
    }

    numEntries = c;

    // check if directory contains PRG with same name
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
// get filename for thumbnail of selected folder/program
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
        strcpy(strThumbFile, thumbDir);
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

//----------------------------------------------------------------------------
// get default filename for thumbnails (folders, programs etc)
char* getDefaultThumbnailFileName(int selectedIndex)
{
    static char strThumbFile[80];
    if (isLocalMode)
        return NULL;

    strcpy(strThumbFile, thumbDir);
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

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
        strcpy(strMetaFile, metaDir);
        strcat(strMetaFile, "/");
        strcat(strMetaFile, arrDirectory[selectedIndex].name);
    }
    strcat(strMetaFile, ".inf");    

    return strMetaFile;
}

//----------------------------------------------------------------------------
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

    showTextWrapped(szDesc, 32, 19, 36, 6);

}

//----------------------------------------------------------------------------
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
            case 0x85: // F1 key
                hideAllSprites();
                showIntroScreen();
                waitKeypress();
                clrscr();
                drawLayout();
                getDirectoryListSorted();    
                continue;
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

//----------------------------------------------------------------------------
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
    textcolor(0);

    strcpy(baseDir, getCurrentDirectory());
    //if (baseDir[0] > '9' && baseDir[1] != '.')
    //    baseDir[0]+=32;


    if (baseDir[0]=='/')
        printf(" %-25s", baseDir);
    else
        printf(" ./%-23s", baseDir);

    textcolor(0);
    bgcolor(15);

    gotoxy(4,23);
    printf(" F1 for instructions      ");
                                      //
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
    printf("Launcher V1.0 (c) AndyMt");
}

//----------------------------------------------------------------------------
// initialize stuff
void init()
{
    struct ini_section *sections = NULL;
    struct ini_section *curr_section = sections;
    char* szValue=NULL;

    bgcolor(6);
    textcolor(6);

    baseDir[0]=0;
    strcpy(baseDir, "/");
    strcpy(launcherDir, "/");
    strcpy(startDir, "/");
    strcpy(thumbDir, "");
    strcpy(metaDir, "");
    isLocalMode = true;
    
    while (kbhit())
    { cgetc(); }
    
    sections = read_ini(INI_FILE);
    if (sections) // ini file is there, get values
    {
        szValue = get_ini_property(sections, "localmode")->value;
        isLocalMode = szValue[0]=='1' || szValue[0]=='y';
        szValue = get_ini_property(sections, "thumbDir")->value;
        if (szValue)
        {
            strcpy(thumbDir, szValue);
        }
        szValue = get_ini_property(sections, "metaDir")->value;
        if (szValue)
        {
            strcpy(metaDir, szValue);
        }
        szValue = get_ini_property(sections, "startDir")->value;
        if (szValue)
        {
            strcpy(startDir, szValue);
        }
        szValue = get_ini_property(sections, "launcherDir")->value;
        if (szValue)
        {
            strcpy(launcherDir, szValue);
        }
    }

    // setup hardware
    SetupScreenMode();
    joy_install(cx16_std_joy);

    if (startDir[0])
        changeDir(startDir);

    // empty keyboard buffer first
    while (kbhit())
    { cgetc(); }

    // prepare directory buffers
    setupDirectory(&DirList, 50);
    setupDirectory(&SubList, 10);
}

//----------------------------------------------------------------------------
// main program
int main(int argc, char *argv[])
{
    char key=argc;
    int index=0;
    char prgName[40];
    char prgPath[80];
    char* szName=argv[0];

    init();

    do
    {
        clrscr();
        drawLayout();
        getDirectoryListSorted();    
        index = navigate();
        if (index == -1)
        {
            // reset machine
/*            asm ("lda #0");
            asm ("sta $9F60");
            asm ("jsr $FF81");
            asm ("jsr $FF8A");
            asm ("sec");
            asm ("jsr $FF47");*/
            restoreScreenmode();
            cbm_k_clall();
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

                szName = getCurrentDirectory();
                if (strstr(startDir, szName)==0 || arrDirectory[index].name[0]!='.')
                    changeDir(arrDirectory[index].name);
            }
        }
        else // launch prg in current directory
        {
            launchPrg(prgName); 
            return 0;
        }
    } 
    while (true);

    return 0;
}
