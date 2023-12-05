REM "C:\Program Files\Josip Medved\VHD Attach\VhdAttach.exe" /attach sd8.vhd
ping 127.0.0.1 -n 2 > nul
REM rmdir F:\games /s /q
REM rmdir F:\games-demo /s /q
REM rmdir F:\games-prg /s /q
REM xcopy out\TestData\*.* F:\ /s /d /y
rem xcopy out\TestData\*.PRG F:\ /y
rem xcopy out\TestData\*.X16 F:\ /y
ping 127.0.0.1 -n 2 > nul
REM "C:\Program Files\Josip Medved\VHD Attach\VhdAttach.exe" /detach sd8.vhd
ping 127.0.0.1 -n 3 > nul
(echo LOAD"LAUNCHER.PRG",8) | clip
ping 127.0.0.1 -n 1 > nul
D:\Projects\RetroComputing\X16\emu\x16emu.exe -sdcard sd8.vhd -joy1 -scale 2 -keymap de-ch -echo >out.log
REM x16emu.exe -sdcard test.vhd -ymstrict -scale 2 -keymap de-ch -echo >out.log
