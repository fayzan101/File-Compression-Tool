@echo off
cd /d %~dp0

rem Collect all .cpp files under src recursively and compile
setlocal enabledelayedexpansion
set "SRCFILES="
for /R src %%f in (*.cpp) do (
    set "SRCFILES=!SRCFILES! "%%f""
)



rem Suppress warnings (-w) but keep debug info (-g) and optimize moderately (-O2)
g++ %SRCFILES% -I include -std=c++17 -O2 -g -w -o main.exe
IF %ERRORLEVEL% NEQ 0 (
    echo.
    pause
    exit /b
)

echo.
main.exe

echo.
pause
