#include "launch.h"
#include <conio.h>
#include <stdio.h>

#include "dirtools.h"
#include "graphics.h"


//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
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
