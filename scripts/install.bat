@echo off
REM Arabic Programming Language - Installation Script
REM Version: 1.0.1
REM Date: January 5, 2026

echo ========================================
echo  مركز تثبيت اللغة العربية
echo  Arabic Language Installation Center
echo ========================================

REM Check if running as administrator
net session >nul 2>&1
if %errorLevel% == 0 (
    echo Running with administrator privileges.
) else (
    echo WARNING: Not running as administrator.
    echo Some installation features may not work properly.
    echo.
)

echo Checking system requirements...
echo Windows version:
ver
echo.

REM Check if already installed
if exist "C:\ArabicLang" (
    echo WARNING: Arabic Language appears to be already installed at C:\ArabicLang
    set /p choice="Do you want to reinstall? (y/n): "
    if not "!choice!"=="y" if not "!choice!"=="Y" (
        echo Installation cancelled.
        pause
        exit /b 0
    )
)

echo Select installation location:
echo 1. Install for current user only (recommended)
echo 2. Install for all users (requires administrator)
echo 3. Custom location
echo.

set /p install_type="Enter your choice (1-3): "

if "%install_type%"=="1" (
    set INSTALL_DIR=%USERPROFILE%\ArabicLang
    echo Installing to user directory: %INSTALL_DIR%
) else if "%install_type%"=="2" (
    REM Check if running as administrator for system installation
    net session >nul 2>&1
    if %errorLevel% == 0 (
        set INSTALL_DIR=C:\Program Files\ArabicLang
        echo Installing to system directory: %INSTALL_DIR%
    ) else (
        echo WARNING: System installation requires administrator privileges.
        echo Falling back to user installation.
        set INSTALL_DIR=%USERPROFILE%\ArabicLang
        echo Installing to user directory: %INSTALL_DIR%
    )
) else if "%install_type%"=="3" (
    set /p INSTALL_DIR="Enter custom installation path: "
    echo Installing to custom location: %INSTALL_DIR%
) else (
    echo Invalid choice. Exiting.
    pause
    exit /b 1
)

echo.
echo Installation directory: %INSTALL_DIR%
echo.

REM Create installation directory
if not exist "%INSTALL_DIR%" mkdir "%INSTALL_DIR%"

echo [1/4] Copying core files...
if not exist "bin\arabic_compiler.exe" (
    echo ERROR: arabic_compiler.exe not found in bin directory!
    echo Please run build_release.bat first.
    echo Looking for files in current directory...
    dir /b
    pause
    exit /b 1
)

echo Copying bin directory...
xcopy /E /I /Y "bin" "%INSTALL_DIR%\bin\" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to copy bin directory
    echo Source: bin
    echo Destination: %INSTALL_DIR%\bin\
    pause
    exit /b 1
)

echo Copying libraries directory...
xcopy /E /I /Y "libraries" "%INSTALL_DIR%\libraries\" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to copy libraries directory
    echo Source: libraries
    echo Destination: %INSTALL_DIR%\libraries\
    pause
    exit /b 1
)

echo Copying tools directory...
xcopy /E /I /Y "tools" "%INSTALL_DIR%\tools\" >nul 2>&1
if errorlevel 1 (
    echo ERROR: Failed to copy tools directory
    echo Source: tools
    echo Destination: %INSTALL_DIR%\tools\
    pause
    exit /b 1
)
echo Core files copied successfully.

echo [2/4] Setting up examples...
if exist "examples" (
    echo Copying examples directory...
    xcopy /E /I /Y "examples" "%INSTALL_DIR%\examples\" >nul 2>&1
    if errorlevel 1 (
        echo ERROR: Failed to copy examples directory
        echo Source: examples
        echo Destination: %INSTALL_DIR%\examples\
        pause
        exit /b 1
    )
    echo Examples copied successfully.
) else (
    echo No examples found.
)

echo [3/4] Setting up documentation...
if exist "docs" (
    echo Copying docs directory...
    xcopy /E /I /Y "docs" "%INSTALL_DIR%\docs\" >nul 2>&1
    if errorlevel 1 (
        echo ERROR: Failed to copy docs directory
        echo Source: docs
        echo Destination: %INSTALL_DIR%\docs\
        pause
        exit /b 1
    )
    echo Documentation copied successfully.
) else (
    echo No documentation found.
)

echo [4/4] Configuring environment...
echo Adding Arabic Language to PATH...

REM Add to PATH for current user
setx PATH "%PATH%;%INSTALL_DIR%\bin" >nul 2>&1
if %errorLevel% == 0 (
    echo PATH updated for current user.
) else (
    echo WARNING: Could not update PATH automatically.
    echo Please manually add %INSTALL_DIR%\bin to your PATH.
)

REM Create desktop shortcut
echo Creating desktop shortcut...
set SHORTCUT_PATH="%USERPROFILE%\Desktop\Arabic Compiler.lnk"
if exist %SHORTCUT_PATH% del %SHORTCUT_PATH%

REM Create a basic batch file to run the compiler
echo @echo off > "%INSTALL_DIR%\run_compiler.bat"
echo "%INSTALL_DIR%\bin\arabic_compiler.exe" %%* >> "%INSTALL_DIR%\run_compiler.bat"

echo Creating start menu entries...
if not exist "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Arabic Language" (
    mkdir "%APPDATA%\Microsoft\Windows\Start Menu\Programs\Arabic Language" 2>nul
)

echo Creating uninstaller...
echo @echo off > "%INSTALL_DIR%\uninstall.bat"
echo echo Uninstalling Arabic Language... >> "%INSTALL_DIR%\uninstall.bat"
echo rmdir /S /Q "%INSTALL_DIR%" >> "%INSTALL_DIR%\uninstall.bat"
echo echo Arabic Language uninstalled successfully. >> "%INSTALL_DIR%\uninstall.bat"
echo pause >> "%INSTALL_DIR%\uninstall.bat"

echo ========================================
echo Installation completed successfully!
echo ========================================
echo.
echo Arabic Language has been installed to:
echo %INSTALL_DIR%
echo.
echo What's next:
echo 1. Run: setup_path.bat (to add to PATH)
echo 2. Open a new command prompt
echo 3. Type: arabic_compiler --help
echo 4. Try the examples in: %INSTALL_DIR%\examples
echo.
echo Development tools available:
echo - arabic_debugger    (Interactive debugger)
echo - arabic_formatter   (Code formatter)
echo - arabic_renamer     (Variable renamer)
echo.
echo Documentation: %INSTALL_DIR%\docs
echo.
echo To uninstall, run: %INSTALL_DIR%\uninstall.bat
echo.
echo ========================================
echo Welcome to Arabic Programming! 🌟
echo ========================================

pause
