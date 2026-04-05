@echo off
setlocal enabledelayedexpansion

echo ============================================================
echo     Arabic Compiler Full Build v1.0
echo ============================================================
echo.

set COMPILER=g++
set FLAGS=-std=c++17 -O2 -DNDEBUG -finput-charset=UTF-8 -fexec-charset=UTF-8

REM Include paths
set INCLUDES=-Isrc -Isrc/core -Isrc/runtime -Isrc/stdlib -Isrc/stdlib/io -Isrc/stdlib/collections -Isrc/stdlib/math -Isrc/api -Isrc/modules -Isrc/modules/ai -Isrc/modules/core -Isrc/modules/edu -Isrc/modules/enterprise -Isrc/modules/graphics -Isrc/modules/mobile -Isrc/modules/ui -Isrc/modules/web -Isrc/utils -Isrc/utils/core -Isrc/utils/debug -Isrc/include -Isrc/libraries

REM Libraries
set LIBS=-lstdc++ -lm -luser32 -lgdi32 -lshell32 -ladvapi32 -lws2_32 -lwinhttp -lgdiplus -lole32 -loleaut32 -luuid -lstrmiids -lcomdlg32 -lcomctl32 -lwinmm

REM Output
set OUTPUT=..\arabic_v18_final.exe

echo Building compiler...
echo.

REM Collect all source files
set SOURCES=..\src\main.cpp
for /r ..\src\core %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\runtime %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\stdlib\io %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\stdlib\collections %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\stdlib\math %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\utils\core %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\utils\debug %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\modules\ai %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\modules\graphics %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\modules\ui %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\modules\web %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\modules\mobile %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\modules\edu %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\modules\enterprise %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\modules\core %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"
for /r ..\src\api %%f in (*.cpp) do set SOURCES=!SOURCES! "%%f"

echo Compiling with all modules...
%COMPILER% %FLAGS% %INCLUDES% -o %OUTPUT% %SOURCES% %LIBS%

if %errorlevel% equ 0 (
    echo.
    echo ============================================================
    echo     BUILD SUCCESSFUL!
    echo ============================================================
) else (
    echo.
    echo ============================================================
    echo     BUILD FAILED!
    echo ============================================================
)

pause
