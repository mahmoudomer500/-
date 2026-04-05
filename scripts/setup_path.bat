@echo off
REM Arabic Programming Language - PATH Setup Script
REM Adds the Arabic Language bin directory to PATH environment variable

echo ========================================
echo  إعداد مسار اللغة العربية
echo  Arabic Language PATH Setup
echo ========================================

REM Get the directory where this script is located
set "SCRIPT_DIR=%~dp0"
set "SCRIPT_DIR=%SCRIPT_DIR:~0,-1%"
set "BIN_DIR=%SCRIPT_DIR%\bin"

echo Current directory: %SCRIPT_DIR%
echo Bin directory: %BIN_DIR%
echo.

REM Check if bin directory exists
if not exist "%BIN_DIR%" (
    echo ERROR: Bin directory not found at: %BIN_DIR%
    echo Please ensure the Arabic Language is properly installed.
    pause
    exit /b 1
)

REM Check if arabic_compiler.exe exists
if not exist "%BIN_DIR%\arabic_compiler.exe" (
    echo ERROR: arabic_compiler.exe not found in bin directory.
    echo Please ensure the compiler is properly installed.
    pause
    exit /b 1
)

echo Found Arabic compiler at: %BIN_DIR%\arabic_compiler.exe
echo.

REM Check current PATH
echo Checking current PATH...
echo %PATH% | findstr /C:"%BIN_DIR%" >nul
if %errorlevel% == 0 (
    echo ✅ Arabic Language bin directory is already in PATH.
    echo.
    echo Current PATH includes: %BIN_DIR%
    echo.
    goto :test_compiler
)

echo Arabic Language bin directory not found in PATH.
echo Adding to user PATH...
echo.

REM Add to user PATH (works without admin privileges)
echo Setting PATH environment variable...
setx PATH "%PATH%;%BIN_DIR%"
if %errorlevel% == 0 (
    echo ✅ Successfully added Arabic Language to user PATH.
    echo.
    echo New PATH entry: %BIN_DIR%
    echo.
    echo NOTE: You may need to restart command prompt windows
    echo for the PATH changes to take effect.
    echo.
    echo IMPORTANT: Close this command prompt and open a new one!
    echo.
) else (
    echo ❌ Failed to update PATH automatically.
    echo Error code: %errorlevel%
    echo.
    echo To add manually:
    echo 1. Press Windows + R, type "sysdm.cpl" and press Enter
    echo 2. Click "Advanced" tab
    echo 3. Click "Environment Variables"
    echo 4. Under "User variables for [username]", find "Path"
    echo 5. Click "Edit" and add: %BIN_DIR%
    echo 6. Click OK to save all dialogs
    echo.
    echo Alternatively, use the full path for now:
    echo "%BIN_DIR%\arabic_compiler.exe" --help
    echo.
)

:test_compiler
echo Testing compiler access...
echo.

REM Test if compiler is accessible
"%BIN_DIR%\arabic_compiler.exe" --version >nul 2>&1
if %errorlevel% == 0 (
    echo ✅ Compiler test successful!
    echo Arabic Language is ready to use.
    echo.
    echo Try: arabic_compiler --help
) else (
    echo ⚠️  Compiler test failed.
    echo You may need to restart your command prompt.
    echo Or use the full path: "%BIN_DIR%\arabic_compiler.exe"
    echo.
)

echo ========================================
echo PATH setup completed!
echo ========================================
echo.
echo Installation directory: %SCRIPT_DIR%
echo Bin directory: %BIN_DIR%
echo.
echo Available commands:
echo - arabic_compiler    (Main compiler)
echo - arabic_debugger    (Interactive debugger)
echo - arabic_formatter   (Code formatter)
echo - arabic_renamer     (Variable renamer)
echo.
echo Example usage:
echo arabic_compiler hello.عربي -o hello.exe
echo.
echo ========================================

pause
