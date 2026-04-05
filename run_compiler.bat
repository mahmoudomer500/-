@echo off
chcp 65001 > nul
cd /d "%~dp0اللغة العربية الكاملة\executables"
arabic_compiler.exe ..\..\src\core\SelfHostingCompiler.arabic
pause
