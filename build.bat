@echo off

set "GAME=C:\Program Files (x86)\Steam\steamapps\common\Mewgenics"
set "OUT=build"

if /I "%~1"=="test" goto :test
if /I "%~1"=="mod" goto :mod
if /I "%~1"=="all" goto :all

REM Default: tests + mod
goto :all


REM ============================================================
REM Tests
REM ============================================================

:test

echo.
echo ===== BUILDING TESTS =====
echo.

if not exist "%OUT%" mkdir "%OUT%"

cl /EHsc /std:c++20 ^
    Native\Save\Analysis\CatInspector.cpp ^
    Native\Save\Analysis\BreedingAnalyzer.cpp ^
    Tests\CatInspectorTests.cpp ^
    Tests\CatSearchTests.cpp ^
    Tests\TestMain.cpp ^
    /Fe:%OUT%\CatManagerTests.exe

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ===== TEST BUILD FAILED =====
    pause
    exit /b %ERRORLEVEL%
)

echo.
echo ===== RUNNING TESTS =====
echo.

"%OUT%\CatManagerTests.exe"

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ===== TESTS FAILED =====
    pause
    exit /b %ERRORLEVEL%
)

echo.

if /I "%~1"=="test" (
    pause
)

exit /b 0


REM ============================================================
REM Full build
REM ============================================================

:all

call "%~f0" test

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ===== BUILD STOPPED: TESTS FAILED =====
    exit /b %ERRORLEVEL%
)

call "%~f0" mod

exit /b %ERRORLEVEL%


REM ============================================================
REM Mod DLL
REM ============================================================

:mod

set "GAME=C:\Program Files (x86)\Steam\steamapps\common\Mewgenics"
set "OUT=build"

echo.
echo ===== BUILDING CATMANAGER =====
echo.

if exist "%GAME%\mod\_logs\catmanager.log" (
    del "%GAME%\mod\_logs\catmanager.log"
)

if not exist "%OUT%" (
    mkdir "%OUT%"
)

cl /LD /EHsc /std:c++20 ^
    Native\CatManager.cpp ^
    Native\Globals.cpp ^
    Native\Mod.cpp ^
    Native\Logger.cpp ^
    Native\Hooks.cpp ^
    Native\Save\SaveLocator.cpp ^
    Native\Save\Repository\SaveParser.cpp ^
    Native\Save\Decoder\CatDecoder.cpp ^
    Native\Save\Compression\Lz4.cpp ^
    Native\Save\Analysis\FamilyBuilder.cpp ^
    Native\Save\Analysis\BreedingAnalyzer.cpp ^
    Native\Save\Analysis\CatInspector.cpp ^
    Native\Save\Pedigree\PedigreeParser.cpp ^
    Native\UI\CatManagerUI.cpp ^
    Native\UI\CatManagerSearch.cpp ^
    Native\UI\CatManagerBreeding.cpp ^
    Native\UI\CatManagerView.cpp ^
    Native\UI\mew_ui_api.c ^
    Native\sqlite3.c ^
    /Fe:%OUT%\CatManager.dll ^
    /link user32.lib kernel32.lib

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ===== BUILD FAILED =====
    pause
    exit /b %ERRORLEVEL%
)

REM ------------------------------------------------------------
REM Deploy DLL
REM ------------------------------------------------------------

copy /Y "%OUT%\CatManager.dll" "%GAME%\mods\CatManager.dll"

if not exist "%GAME%\mods\CatManager" (
    mkdir "%GAME%\mods\CatManager"
)

copy /Y "%OUT%\CatManager.dll" "%GAME%\mods\CatManager\CatManager.dll"

REM ------------------------------------------------------------
REM Deploy text
REM ------------------------------------------------------------

if not exist "%GAME%\mods\CatManager\data\text" (
    mkdir "%GAME%\mods\CatManager\data\text"
)

if exist "data\text\combined.csv.append" (
    copy /Y "data\text\combined.csv.append" "%GAME%\mods\CatManager\data\text\combined.csv.append"
)

REM ------------------------------------------------------------
REM Deploy SWF at game root
REM ------------------------------------------------------------

if not exist "%GAME%\swfs" (
    mkdir "%GAME%\swfs"
)

if exist "swfs\house_ui_test.swf" (
    copy /Y "swfs\house_ui_test.swf" "%GAME%\mods\CatManager\swfs\house_ui_test.swf"
)

if exist "swfs\swflist.gon.append" (
    copy /Y "swfs\swflist.gon.append" "%GAME%\mods\CatManager\swfs\swflist.gon.append"
)

REM ------------------------------------------------------------
REM Launch
REM ------------------------------------------------------------

start "" "%GAME%\Mewgenics.exe" -enable_debugconsole true -modpaths "%GAME%\mods\CatManager"

echo.
echo ===== DONE =====
echo.

pause
exit /b 0