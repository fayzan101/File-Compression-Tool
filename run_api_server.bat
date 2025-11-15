@echo off
echo Starting Crow API Server for HuffmanCompressor...
echo.
echo Server will be available at: http://localhost:8080
echo.
echo Press Ctrl+C to stop the server
echo.

REM Create necessary directories
if not exist "uploads" mkdir uploads
if not exist "compressed" mkdir compressed
if not exist "decompressed" mkdir decompressed

REM Run the server
api_server.exe
