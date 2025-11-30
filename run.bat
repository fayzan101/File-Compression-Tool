@echo off
cd /d %~dp0
setlocal enabledelayedexpansion

rem Collect all .cpp files except excluded ones
set "SRCFILES="
for /R src %%f in (*.cpp) do (
    echo %%f | findstr /I /C:"profiler_unused.cpp" /C:"RestServer.cpp" /C:"api_server.cpp" >nul
    if errorlevel 1 (
        set "SRCFILES=!SRCFILES! "%%f""
    )
)

rem Check if main.exe exists
if not exist main.exe goto build

rem Check if any .cpp or .h file is newer than main.exe
set "REBUILD=0"
for /R src %%f in (*.cpp) do (
    for %%x in (main.exe) do if %%~tf GTR %%~tx (set REBUILD=1)
)
for /R include %%f in (*.h) do (
    for %%x in (main.exe) do if %%~tf GTR %%~tx (set REBUILD=1)
)
if !REBUILD! == 1 goto build

goto run

:build
echo Building project...
g++ !SRCFILES! -I include -std=c++17 -O2 -g -w -o main.exe
IF %ERRORLEVEL% NEQ 0 (
    echo Build failed.
    exit /b
)

:run
echo.
echo Starting API Server in background...
start "HuffmanCompressor API Server" cmd /k "api_server.exe"

echo.
echo Running main CLI application...
main.exe
echo.
