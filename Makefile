.PHONY: all clean

LIBRARIES := -L libX16/lib 
INCLUDES := -I libX16/include 

SRC = \
	src/globals.c src/dirtools.c src/inifile.c src/utils.c src/intro.c src/launch.c src/main.c src/graphics.c src/util.asm

OBJ = $(SRC:%.c=%.o)

#.c.o:
#	cl65 -t cx16 -I $(INCLUDE) -Oi $<

main: src/main.c  $(SRC)
	cl65 -t cx16 $(INCLUDES) $(LIBRARIES) -C src/mycx16.cfg -Oi -Or -Os -O -o out/launcher.prg $(SRC) cx16.lib cx16m-std.mou cx16j-std.joy
	#cc65 -t cx16 $(INCLUDES) -Oi -Or -Os -O src/main.c

clean:
	del .\out\*.prg .\src\*.o

copy:
	@#cp .\out\launcher.prg .\out\Testdata\LAUNCHER.PRG
	@#cp .\out\launcher.prg .\out\Testdata\AUTOBOOT.X16

	@cp .\out\launcher.prg .\out\Testdata\LAUNCHER\LAUNCHER.PRG
	@cp .\out\launcher.prg .\out\Testdata\LAUNCHER\AUTOBOOT.X16
	@rm -f .\out\Testdata\LAUNCHER\.LAUNCHER.TMP

	@cp .\out\launcher.prg E:\APPS\LAUNCHER.PRG
	@rm -f E:\LAUNCHER.DATA\.LAUNCHER.TMP
	@rm -f E:\.LAUNCHER.TMP

all: bins main copy

bins: 
