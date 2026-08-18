@echo off
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
cd /d "%~dp0"
cl /EHsc /O2 /std:c++17 WhiteSquareClicker.cpp /Fe:WhiteSquareClicker.exe user32.lib gdi32.lib
pause
