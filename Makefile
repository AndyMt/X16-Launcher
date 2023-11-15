.PHONY: all clean

LIBRARIES := -L libX16/lib 
INCLUDES := -I libX16/include 

SRC = \
	src/main.c src/graphics.c src/util.asm

OBJ = $(SRC:%.c=%.o)

#.c.o:
#	cl65 -t cx16 -I $(INCLUDE) -Oi $<

main: src/main.c  $(SRC)
	cl65 -t cx16 $(INCLUDES) $(LIBRARIES) -C src/mycx16.cfg -Oi -o out/launcher.prg $(SRC) cx16.lib cx16m-std.mou cx16j-std.joy

clean:
	del .\out\*.prg .\src\*.o

copy:
	cp .\out\launcher.prg .\out\AUTOBOOT.X16
	cp .\out\launcher.prg E:\AUTOBOOT.X16

all: bins main copy

bins: 
