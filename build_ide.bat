@echo off
setlocal EnableDelayedExpansion

echo ========================================
echo  Al-Biruni IDE Build Script
echo ========================================
echo.

where g++ >nul 2>&1
if %ERRORLEVEL% NEQ 0 (
    echo ERROR: g++ not found in PATH
    echo Install MinGW64 and add to PATH
    pause
    exit /b 1
)
echo OK: g++ found
g++ --version | findstr /i "g++"
echo.

if not exist "build_release\Release" mkdir build_release\Release

if exist "build_release\Release\AlBiruniIDE.exe" (
    echo [0/3] Cleaning old build...
    del /q "build_release\Release\AlBiruniIDE.exe" >nul 2>&1
)

echo [1/3] Compiling IDE (62 files)...
echo.

g++ @build_ide.rsp

if %ERRORLEVEL% NEQ 0 (
    echo.
    echo ========================================
    echo  BUILD FAILED!
    echo ========================================
    pause
    exit /b 1
)

echo.
echo [2/3] Verifying build...
if not exist "build_release\Release\AlBiruniIDE.exe" (
    echo File not created!
    pause
    exit /b 1
)

for %%A in ("build_release\Release\AlBiruniIDE.exe") do (
    set SIZE=%%~zA
    set /a SIZE_KB=!SIZE! / 1024
)

echo.
echo [3/3] Done!
echo.
echo ========================================
echo  Build Successful!
echo ========================================
echo  File: build_release\Release\AlBiruniIDE.exe
echo  Size: !SIZE_KB! KB
echo ========================================
pause
