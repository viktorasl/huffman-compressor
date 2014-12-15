//
//  main.cpp
//  huffman
//
//  Created by Viktoras Laukevičius on 01/12/14.
//  Copyright (c) 2014 viktorasl. All rights reserved.
//

//  Encoded file: word_size:1B|file_size:4B|left_bits_length:1B|left_bits:2B|tree|content

#include <vector>
#include <deque>
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <set>
#include <algorithm>
#include <queue>
#include <math.h>

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

static const int readBufferSize = 256;

using namespace std;

typedef vector<bool> HuffCode;
typedef map<int, HuffCode> HuffTable;
typedef int HuffValue;
typedef double HuffFrequency;
typedef map<HuffValue, HuffFrequency> HuffFrequenciesTable;
typedef HuffFrequenciesTable::iterator HuffFrequenciesIterator;
typedef void (*CallbackType)(int, int);

struct HuffEntry {
    int value = 0;
    double frequency = 0;
    HuffEntry *left = NULL;
    HuffEntry *right = NULL;
};

HuffFrequenciesTable frequencies;
HuffTable tableMap;
char encoded;
int encodedSize;
HuffEntry *root;
int leftBits;
int leftBitsLength;

ofstream* of;
ofstream *in;

#ifdef DEBUG
void print_char_to_binary(char ch)
{
    int i;
    for (i=(sizeof(ch) * 8 - 1);i>=0;i--)
        printf("%d",((ch & (1<<i))>>i));
    printf("\n");
}
#endif

#ifdef DEBUG
void print_binary(long ch, int size)
{
    int i;
    for (i = size - 1; i >= 0; i--) {
        cout << ((ch & (1<<i))>>i);
    }
}
#endif

struct Comp{
    bool operator()(const HuffEntry *a, const HuffEntry *b){
        return a->frequency > b->frequency;
    }
};

void generateCodes(HuffEntry *root, HuffTable* populateTable, HuffCode code) {
    if (!root->left && !root->right) {
        populateTable->insert(std::pair<int, HuffCode>(root->value, code));
    }
    if (root->left) {
        HuffCode newCode(code);
        newCode.push_back(0);
        generateCodes(root->left, populateTable, newCode);
    }
    if (root->right) {
        HuffCode newCode(code);
        newCode.push_back(1);
        generateCodes(root->right, populateTable, newCode);
    }
}

#ifdef DEBUG
void around(HuffEntry *root, string code) {
    if (!root->left && !root->right) {
//        cout << "value=" << root->value << " freq=" << root->frequency << " code=" << code << endl;
    }
    if (root->left) {
        stringstream ss;
        ss << code << '0';
        around(root->left, ss.str());
    }
    if (root->right) {
        stringstream ss;
        ss << code << '1';
        around(root->right, ss.str());
    }
}
#endif

#ifdef DEBUG
void printCodesTree(HuffEntry *root, string code)
{
    if (!root->left && !root->right) {
        cout << "value=" << root->value << " code=" << code << endl;
    }
    if (root->left) {
        stringstream ss;
        ss << code << '0';
        printCodesTree(root->left, ss.str());
    }
    if (root->right) {
        stringstream ss;
        ss << code << '1';
        printCodesTree(root->right, ss.str());
    }
}
#endif

void analyze(char *filename, int wordLength, CallbackType callback, CallbackType lastBits)
{
    int wordInt = 0;
    int wordFill = 0;
    
    ifstream file (filename , ifstream::in|ifstream::binary);
    char* readBuffer = new char[readBufferSize];
    
    // Building leaves
    while (file.good()) {
        
        size_t length = file.read(readBuffer, readBufferSize).gcount();
        
        // Each character in buffer
        int i;
        for (i = 0; i < length; i++) {
            char ch = readBuffer[i];
            
            // Each bit in character
            int j;
            for (j = sizeof(char) * 8 - 1; j >= 0; j--) {
                bool bit = (ch & (1 << j)) >> j;
                
                if (wordFill > 0) {
                    wordInt <<= 1;
                }
                
                wordInt |= bit;
                wordFill++;
                
                // Finished composing word of wordLength bits, making HuffEntry leaf
                if (wordFill == wordLength) {
                    callback(wordFill, wordInt);
                    
                    // Resetting
                    wordInt = 0;
                    wordFill = 0;
                    
                }
            }
        }
    }
    
    if (wordFill > 0) {
        if (lastBits) {
            lastBits(wordFill, wordInt);
        }
    }
    
    free(readBuffer);
    file.close();
}

void gather(int size, int word) {
    frequencies[word]++;
}

void leftBitsHandler(int size, int word)
{
    leftBits = word;
    leftBitsLength = size;
}

void encd(int size, int word)
{
    HuffCode cd = tableMap[word];
    int i;
    for (i = 0; i < cd.size(); i++) {
        encoded |= cd[i];
        encodedSize++;
        
        if (encodedSize == 8) {
            of->put(encoded);
            encoded = 0;
            encodedSize = 0;
        } else {
            encoded <<= 1;
        }
        
    }
}

void buildingTreeAppend(bool bit, char *ch, int *fill)
{
    (*ch) |= bit;
    (*fill)++;
    
    if (*fill == 8) {
        of->put(*ch);
        *ch = 0;
        *fill = 0;
    } else {
        (*ch) <<= 1;
    }
}

void writeTreeToFile(int wordLength, HuffEntry *root, char *ch, int *fill)
{
    if (!root->left && !root->right) {
        buildingTreeAppend(1, ch, fill);
        
        int i;
        for (i = wordLength - 1; i >= 0; i--) {
            bool bit = (root->value & (1 << i)) >> i;
            buildingTreeAppend(bit, ch, fill);
        }
        
    } else {
        buildingTreeAppend(0, ch, fill);
    }
    
    if (root->left) {
        writeTreeToFile(wordLength, root->left, ch, fill);
    }
    if (root->right) {
        writeTreeToFile(wordLength, root->right, ch, fill);
    }
}

void vl_encode(char *inFileName, char *outFileName, int wordLength)
{
    of = new ofstream(outFileName, ofstream::out|ofstream::binary);
    
    analyze(inFileName, wordLength, &gather, &leftBitsHandler);
    
    cout << "-- gathered frequencies" << endl;
    
    priority_queue<HuffEntry *, vector<HuffEntry *>, Comp> table;
    
    // Building priority queue
    for(HuffFrequenciesIterator iterator = frequencies.begin(); iterator != frequencies.end(); iterator++) {
        HuffEntry *h = new HuffEntry();
        h->value = iterator->first;
        h->frequency = iterator->second;
        
        table.push(h);
    }
    
    cout << "-- built priority queue" << endl;
    
    while (table.size() > 1) {
        HuffEntry* f = table.top();
        table.pop();
        HuffEntry* s = table.top();
        table.pop();
        
        HuffEntry *up = new HuffEntry();
        up->value = -1;
        up->frequency = f->frequency + s->frequency;
        up->left = f;
        up->right = s;
        
        table.push(up);
    }
    
    cout << "-- built tree" << endl;
    
    // Saving word length in 1 Byte
    of->put((char)wordLength);
    
    cout << "-- saved word length" << endl;
    
    // Saving file size in 4 Bytes
    ifstream file (inFileName, ifstream::in|ifstream::binary);
    file.seekg(0, file.end);
    long fileSize = file.tellg();
    int i;
    for (i = 3; i >= 0; i--) {
        char ch = fileSize >> (i * 8);
        of->put(ch);
    }
    file.seekg(0, file.beg);
    file.close();
    
    cout << "-- saved original file size" << endl;
    
    // Saving left bits length in 1 Byte
    of->put((unsigned char)leftBitsLength);
    
    // Saving left bits
    unsigned int bytesCount = ceil((float)leftBitsLength / 8);
    for (i = bytesCount - 1; i >= 0; i--) {
        char leftBitsCh = (leftBits >> (i * 8)) & 0xFF;
        of->put(leftBitsCh);
    }
    
    cout << "-- saved left bits" << endl;
    
    // Saving tree to file
    char ch = 0;
    int chFill = 0;
    writeTreeToFile(wordLength, table.top(), &ch, &chFill);
    if (chFill > 0) {
        ch <<= (8 - chFill - 1);
        of->put(ch);
    }
    
    cout << "-- saved tree" << endl;
    
    root = table.top();
    
    HuffCode code;
    generateCodes(root, &tableMap, code);

    analyze(inFileName, wordLength, &encd, NULL);
    
    // If there are some bits left uncompressed,
    // extending it with 0 bits and writing to file
    if (encodedSize != 0) {
        int shift = 8 - encodedSize - 1;
        encoded <<= shift;
        of->put(encoded);
    }
    
    of->close();
    delete of;
}

void readTree(ifstream *file, const int wordLength, HuffEntry **root, char *ch, int *readBit)
{
    if (*readBit < 0) {
        *ch = file->get();
        *readBit = 7;
    }
    
    bool bit = (*ch & (1 << *readBit)) >> *readBit;
    (*readBit)--;
    
    if (bit) {
        int value = 0;
        int valueLength = 0;
        
        while (valueLength != wordLength) {
            if (*readBit < 0) {
                *ch = file->get();
                *readBit = 7;
            }
            
            bool valueBit = (*ch & (1 << *readBit)) >> *readBit;
            (*readBit)--;
            value |= valueBit;
            valueLength++;
            
            if (valueLength != wordLength) {
                value <<= 1;
            }
        }
        
        HuffEntry *vertex = new HuffEntry();
        vertex->value = value;
        *root = vertex;
        
    } else {
        HuffEntry *vertex = new HuffEntry();
        *root = vertex;
        readTree(file, wordLength, &vertex->left, ch, readBit);
        readTree(file, wordLength, &vertex->right, ch, readBit);
    }
}

void decompresContent(ifstream *file, HuffEntry * const root, int const wordLength, unsigned long const fileSize, char *decoded, int *decodedBits)
{
    char *readBuffer = new char[readBufferSize];
    HuffEntry *it = root;
    unsigned long filledFile = 0;
    
    // Building leaves
    while (file->good()) {
        
        size_t length = file->read(readBuffer, readBufferSize).gcount();
        
        // Each char of buffer
        int i;
        for (i = 0; i < length; i++) {
            char c = readBuffer[i];
            
            // Each bit of read byte
            int j;
            for (j = (sizeof(c) * 8 - 1); j >= 0; j--) {
                bool bit = (c & (1 << j)) >> j;
                
                if (bit == 0 && it->left) {
                    it = it->left;
                } else if (it->right) {
                    it = it->right;
                }
                
                if (!it->left && !it->right && it->value != -1) {
                    
                    // Each bit of decoded code (int)
                    int k;
                    for (k = wordLength - 1;k>=0;k--) {
                        
                        bool codeBit = (it->value & (1 << k)) >> k;
                        *decoded |= codeBit;
                        (*decodedBits)++;
                        
                        // Full char
                        if (*decodedBits == 8) {
                            in->put(*decoded);
                            filledFile++;
                            
                            if (filledFile == fileSize) {
                                return;
                            }
                            
                            *decoded = 0;
                            *decodedBits = 0;
                        } else {
                            (*decoded) <<= 1;
                        }
                    }
                    
                    // Resetting
                    it = root;
                }
                
            }
        }
        
    }
}

void vl_decompress(char *filename, char *outName)
{
    in = new ofstream(outName, ofstream::out|ofstream::binary);
    ifstream file (filename , ifstream::in|ifstream::binary);

    // Word length is compressed in first 1 Byte
    int wordLength = (unsigned char)file.get();
    
    // File size is compressed in 4 Bytes
    char *fileSizeChar = new char[4];
    file.read(fileSizeChar, 4);
    unsigned long fileSize = 0;
    int i;
    for (i = 0; i < 4; i++) {
        fileSize |= (unsigned char)fileSizeChar[i];
        if (i < 3) {
            fileSize <<= 8;
        }
    }
    
    // Left bits length in 1 Byte
    unsigned int leftBitsLength = (unsigned int)file.get();
    unsigned int bytesCount = ceil((float)leftBitsLength / 8);
    char *leftBitsBuffer = new char[bytesCount];
    file.read(leftBitsBuffer, bytesCount);
    int leftBits = 0;
    for (i = 0; i < bytesCount; i++) {
        leftBits |= (unsigned char)leftBitsBuffer[i];
        if (i < bytesCount - 1) {
            leftBits <<= 8;
        }
    }
    
    // Reading tree from file
    HuffEntry *root = NULL;
    char ch = 0;
    int readBit = -1;
    readTree(&file, wordLength, &root, &ch, &readBit);
    
    // Decompressing file content
    char decoded = 0;
    int decodedBits = 0;
    decompresContent(&file, root, wordLength, fileSize, &decoded, &decodedBits);
    
    if (leftBitsLength) {
        int k;
        for (k = leftBitsLength - 1; k >= 0; k--) {
            bool bit = (leftBits & (1 << k)) >> k;
            
            decoded |= bit;
            decodedBits++;
            
            // Full char
            if (decodedBits == 8) {
                in->put(decoded);
                
                decoded = 0;
                decodedBits = 0;
            } else {
                decoded <<= 1;
            }
        }
    }
    
    file.close();
    in->close();
    delete in;
}

int main(int argc, char **argv)
{
    if (argc == 6 && !strcmp(argv[1], "-c") && !strcmp(argv[2], "-w")) {
        unsigned int wordLength = atoi(argv[3]);
        if (wordLength < 2 || wordLength > 16) {
            cout << "Invalid word lenth, must be between 2 and 16" << endl;
            
            return 1;
        }
        
        cout << "-- analyzing..." << endl;
        
        char *inFile = argv[4];
        char *outFile = argv[5];
        
        vl_encode(inFile, outFile, wordLength);
        cout << "Compressed!" << endl;
        
    } else if (argc == 4 && !strcmp(argv[1], "-d")) {
        
        char *inFile = argv[2];
        char *outFile = argv[3];
        
        vl_decompress(inFile, outFile);
        cout << "Decompressed!" << endl;
        
    } else {
        cout << "Invalid arguments! usage:" << endl;
        cout << "\t-c -w <word_length:[2-16]> <filename_to_compress> <compressed_name>" << endl;
        cout << "\t-d <compressed_name> <result_file_name>" << endl;
        
        return 1;
    }
    
    return 0;
}
