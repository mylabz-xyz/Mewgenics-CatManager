@echo off

set "GAME=C:\Program Files (x86)\Steam\steamapps\common\Mewgenics"
set "OUT=..\build"

if exist "%GAME%\mod_logs\catmanager.log" del "%GAME%\mod_logs\catmanager.log"

if not exist "%OUT%" mkdir "%OUT%"

cl /LD /EHsc /std:c++20 ^
CatManager.cpp ^
Globals.cpp ^
Mod.cpp ^
Logger.cpp ^
Hooks.cpp ^
Save\SaveLocator.cpp ^
Save\Repository\SaveParser.cpp ^
Save\Decoder\CatDecoder.cpp ^
Save\Compression\Lz4.cpp ^
Save\Analysis\FamilyBuilder.cpp ^
Save\Pedigree\PedigreeParser.cpp ^
UI\CatManagerUI.cpp ^
UI\mew_ui_api.c ^
sqlite3.c ^
/Fe:%OUT%\CatManager.dll ^
/link user32.lib kernel32.lib

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ===== BUILD FAILED =====
    pause
    exit /b %ERRORLEVEL%
)

REM --------------------------------------------------
REM Deploy DLL
REM --------------------------------------------------

copy /Y "%OUT%\CatManager.dll" "%GAME%\mods\CatManager.dll"

if not exist "%GAME%\mods\CatManager" mkdir "%GAME%\mods\CatManager"

copy /Y "%OUT%\CatManager.dll" "%GAME%\mods\CatManager\CatManager.dll"

REM --------------------------------------------------
REM Deploy text
REM --------------------------------------------------

if not exist "%GAME%\mods\CatManager\data\text" mkdir "%GAME%\mods\CatManager\data\text"

if exist "..\data\text\combined.csv.append" (
    copy /Y "..\data\text\combined.csv.append" "%GAME%\mods\CatManager\data\text\combined.csv.append"
)

REM --------------------------------------------------
REM Deploy SWF at game root
REM --------------------------------------------------

if not exist "%GAME%\swfs" mkdir "%GAME%\swfs"

if exist "..\swfs\house_ui_test.swf" (
    copy /Y "..\swfs\house_ui_test.swf" "%GAME%\swfs\house_ui_test.swf"
)

if exist "..\swfs\swflist.gon.append" (
    copy /Y "..\swfs\swflist.gon.append" "%GAME%\swfs\swflist.gon.append"
)

REM --------------------------------------------------
REM Launch
REM --------------------------------------------------

start "" "%GAME%\Mewgenics.exe"

echo.
echo ===== DONE =====
pause