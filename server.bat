@echo off
echo Building Crow API Server...
g++ -std=c++17 -I./include -I./include/crow -DASIO_STANDALONE src/api_server.cpp src/HuffmanCompressor.cpp src/HuffmanTree.cpp src/BitReader.cpp src/BitWriter.cpp src/Compressor.cpp src/Decompressor.cpp src/FolderCompressor.cpp src/Checksum.cpp src/LZ77.cpp -o api_server.exe -lws2_32 -lmswsock

if %ERRORLEVEL% EQU 0 (
    echo.
    echo Build successful! Run api_server.exe to start the server.
    echo Server will run on http://localhost:8080
) else (
    echo.
    echo Build failed with error code %ERRORLEVEL%
)
pause