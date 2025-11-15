# Use GCC base image
FROM gcc:11

# Install dependencies
RUN apt-get update && apt-get install -y \
    build-essential \
    && rm -rf /var/lib/apt/lists/*

# Set working directory
WORKDIR /app

# Copy all project files
COPY . .

# Create necessary directories
RUN mkdir -p uploads compressed decompressed

# Compile the API server
RUN g++ -std=c++17 \
    -I./include \
    -I./include/crow \
    -DASIO_STANDALONE \
    src/api_server.cpp \
    src/HuffmanCompressor.cpp \
    src/HuffmanTree.cpp \
    src/BitReader.cpp \
    src/BitWriter.cpp \
    src/Compressor.cpp \
    src/Decompressor.cpp \
    src/FolderCompressor.cpp \
    src/Checksum.cpp \
    src/LZ77.cpp \
    -o api_server \
    -lpthread

# Expose port
EXPOSE 8080

# Run the server
CMD ["./api_server"]
