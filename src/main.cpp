#include <iostream>
#include "../include/Compressor.h"
#include "../include/Decompressor.h"

using namespace std;

int main() {
    while (true) {
        cout << "\nHuffmanCompressor Menu:\n";
        cout << "1. Compress file\n";
        cout << "2. Decompress file\n";
        cout << "3. Exit\n";
        cout << "Enter your choice: ";
        string choice;
        getline(cin, choice);
        if (choice == "1") {
            string inPath, outPath;
            cout << "Enter input file path: ";
            getline(cin, inPath);
            cout << "Enter output file path: ";
            getline(cin, outPath);
            Compressor comp;
            bool ok = comp.compress(inPath, outPath);
            if (ok) {
                cout << "Compressed '" << inPath << "' to '" << outPath << "' successfully.\n";
            } else {
                cout << "Compression failed.\n";
            }
        } else if (choice == "2") {
            string inPath, outPath;
            cout << "Enter input file path: ";
            getline(cin, inPath);
            cout << "Enter output file path: ";
            getline(cin, outPath);
            Decompressor decomp;
            bool ok = decomp.decompress(inPath, outPath);
            if (ok) {
                cout << "Decompressed '" << inPath << "' to '" << outPath << "' successfully.\n";
            } else {
                cout << "Decompression failed.\n";
            }
        } else if (choice == "3") {
            cout << "Exiting.\n";
            break;
        } else {
            cout << "Invalid choice. Please enter 1, 2, or 3.\n";
        }
    }
    return 0;
}
