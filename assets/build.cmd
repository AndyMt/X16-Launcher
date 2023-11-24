@ECHO OFF
ECHO [97mBuilding assets:

set PATH=%PATH%;D:\Projects\RetroComputing\X16\tools\aloevera

aloevera create project launcher.av | find "ERROR"

REM X16 default palette
aloevera -p launcher.av palette import x16pal X16DefaultPalette8bpp.png | find "ERROR"

REM Thumbnails
for %%f in (.\Thumbnails128x96\*.png) do (
    CALL :ConvertImage %%~nf, 128, 96, %%f, x16pal
)

REM Build all assets
aloevera -p launcher.av asm -f bin ./output all | find "ERROR"
IF ERRORLEVEL 0 echo [92mOK[0m

REM make all filenames uppercase
setlocal enableDelayedExpansion

pushd .\output\imagesets

for %%f in (*) do (
   set "filename=%%~f"

   for %%A in (A B C D E F G H I J K L M N O P Q R S T U V W X Y Z) do (
      set "filename=!filename:%%A=%%A!"
   )
    ren "%%f" "!filename!" >nul 2>&1
)
endlocal

copy .\output\imagesets\*.bin ..\out\thumbnails\*.ABM >nul
copy .\output\palettes\*.bin ..\out\*.ABM >nul

goto end

:ConvertImage
    ECHO [0m  %~1 [91m
    aloevera -p launcher.av imageset import %~1 %~2 %~3 %~4 | find "ERROR"
    aloevera -p launcher.av imageset format %~1 %~5 8 | find "ERROR"
    echo|set /p="[97m"
EXIT /B 0

:ConvertImageAndPalette
    ECHO [0m  %~1 [91m
    call :Uppercase %~1
    aloevera -p launcher.av palette import %~5 %~4 | find "ERROR"
    aloevera -p launcher.av imageset import %~1 %~2 %~3 %~4 | find "ERROR"
    aloevera -p launcher.av imageset format %~1 %~5 8 | find "ERROR"
    echo|set /p="[97m"
EXIT /B 0

:Uppercase
if not defined %~1 exit /b
for %%a in ("a=A" "b=B" "c=C" "d=D" "e=E" "f=F" "g=G" "h=H" "i=I" "j=J" "k=K" "l=L" "m=M" "n=N" "o=O" "p=P" "q=Q" "r=R" "s=S" "t=T" "u=U" "v=V" "w=W" "x=X" "y=Y" "z=Z" "ä=Ä" "ö=Ö" "ü=Ü") do (
call set %~1=%%%~1:%%~a%%
EXIT /B /0

:end
ECHO [0mDone [97m
EXIT /B 

