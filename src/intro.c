#include "intro.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <cx16.h>
#include <conio.h>
#include <cbm.h>

#include "globals.h"
#include "graphics.h"
#include "inifile.h"

//----------------------------------------------------------------------------
// show intro screen
void showIntroScreen()
{
    uint8_t y;
    static char* szIntro = 
        "This launcher let's you navigate through directories and start programs.\r\n"
        "For a selection of games and programs a thumbnail and short description is shown. "
        "For all others default icons are shown.\r\n\r\n"
        "Navigation by keyboard:\r\n"
        "- up/down:    scroll through directory list\r\n"
        "- left/right: cycle through thumbnails (if multiple)\r\n"
        "- enter:      select directory or start program\r\n"
        "- dot:        to parent directory\r\n"
        "- any letter: jump directly in the list\r\n"
        "- esc:        exit launcher\r\n\r\n"
        "Navigation by joystick/controller:\r\n"
        "- up/down:    scroll through directory list\r\n"
        "- left:       to parent directory\r\n"
        "- any button: select directory or start program\r\n\r\n"
        "This message can be shown again by pressing F1."
        ;

    clrscr();
    while (kbhit())
    { cgetc(); }

    bgcolor(15);
    textcolor(0);

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
    textcolor(0);
    gotoxy(7,23);
    printf(" %-54s ", "Press any key to continue...");

    bgcolor(6);
    textcolor(1);
    showTextWrapped(szIntro, 8,4,53,18);

}

//----------------------------------------------------------------------------
// manage info screen based on temp file content
void manageInfoscreen()
{
    struct ini_section *sections = NULL;
    char* szValue=NULL;
    bool tmpExists = false;

    //show intro screen?
    changeDir(launcherDir);

    sections = read_ini(TMP_FILE);
    if (sections)
        szValue = get_ini_property(sections, "skipintro")->value;
    else
        sections = create_ini_section(sections, "state");
    

    // save flag to skip it from  now on
    if (!szValue || szValue[0]=='0' || szValue[0]=='n')
    {
        showIntroScreen();
        set_ini_property(sections, "skipintro", "1");
        save_ini(TMP_FILE, sections);
        waitKeypress();
    }
}
