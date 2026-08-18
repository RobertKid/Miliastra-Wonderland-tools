RandomNumberSequenceClicker continuous version

This version continuously detects the selected 4x4 region.
When a valid random board is detected, it clicks 1 -> 16 automatically.
It waits for the board to disappear/change before clicking again, so it should not spam-click the same board.

Hotkeys:
F6  = set 4x4 board top-left
F7  = set 4x4 board bottom-right
F8  = start/stop continuous detection
F9  = emergency pause/stop
F10 = click one round once
ESC = exit

Build with MSVC:
call "C:\Program Files\Microsoft Visual Studio\18\Community\Common7\Tools\VsDevCmd.bat" -arch=x64
cd /d "C:\Users\rober\Downloads\RandomNumberSequenceClicker_continuous"
cl /EHsc /O2 /std:c++17 RandomNumberSequenceClicker.cpp /Fe:RandomNumberSequenceClicker.exe user32.lib gdi32.lib

Settings in source:
CLICK_DELAY_MS = delay after every click, currently 3 ms.
DETECT_INTERVAL_MS = how often it scans while continuous mode is ON, currently 20 ms.
AFTER_ROUND_WAIT_MS = wait after finishing 1-16, currently 80 ms.

Change in this version: number 1 is clicked twice before continuing to 2-16. All continuous detection logic is unchanged.
