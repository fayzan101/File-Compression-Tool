@echo off
cd /d %~dp0

rem 
setlocal enabledelayedexpansion
set "SRCFILES="
for /R src %%f in (*.cpp) do (
    echo %%f | findstr /I /C:"profiler_unused.cpp" /C:"RestServer.cpp" >nul
    if errorlevel 1 (
        set "SRCFILES=!SRCFILES! "%%f""
    )
)



rem 
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
