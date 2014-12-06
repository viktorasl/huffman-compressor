// Encoded file: word_size:1B|tree|content

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
#include "HuffCompressor.h"

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

using namespace std;

typedef vector<bool> HuffCode;
typedef map<int, HuffCode> HuffTable;
typedef int HuffValue;
typedef int HuffFrequency;
typedef map<HuffValue, HuffFrequency> HuffFrequenciesTable;
typedef HuffFrequenciesTable::iterator HuffFrequenciesIterator;
typedef void (*CallbackType)(int, int);

struct HuffEntry {
    int value = 0;
    int frequency = 0;
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
int fakeBitsLength;

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
void print_short_to_binary(short ch, int size)
{
    int i;
    int from = MIN(sizeof(ch) * 8, size);
    for (i=from - 1;i>=0;i--)
        printf("%d",((ch & (1<<i))>>i));
    printf("\n");
}
#endif

struct Comp{
    bool operator()(const HuffEntry *a, const HuffEntry *b){
        return a->frequency > b->frequency;
    }
};

void buildMap(HuffEntry *root, HuffTable* populateTable, HuffCode code) {
    if (!root->left && !root->right) {
        populateTable->insert(std::pair<int, HuffCode>(root->value, code));
    }
    if (root->left) {
        HuffCode newCode(code);
        newCode.push_back(0);
        buildMap(root->left, populateTable, newCode);
    }
    if (root->right) {
        HuffCode newCode(code);
        newCode.push_back(1);
        buildMap(root->right, populateTable, newCode);
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
    int bufferSize = 2; // buffer size in bytes
    
    int wordInt = 0;
    int wordFill = 0;
    
    ifstream file (filename , ifstream::in|ifstream::binary);
    char* readBuffer = new char[bufferSize];
    
    // Building leaves
    while (file.good()) {
        
        size_t length = file.read(readBuffer, bufferSize).gcount();
        
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
//            print_char_to_binary(encoded);
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
//        print_char_to_binary(*ch);
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

void vl_encode(char*filename, char*outname, int wordLength)
{
    analyze(filename, wordLength, &gather, &leftBitsHandler);
    
    cout << "analization finished" << endl;
    
    priority_queue<HuffEntry *, vector<HuffEntry *>, Comp> table;
    
    // Building priority queue
    for(HuffFrequenciesIterator iterator = frequencies.begin(); iterator != frequencies.end(); iterator++) {
        HuffEntry *h = new HuffEntry();
        h->value = iterator->first;
        h->frequency = iterator->second;
        
        table.push(h);
    }
    
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
    
//    printCodesTree(table.top(), "");
//    
//    of->put((char)wordLength);
//    char ch = 0;
//    int chFill = 0;
//    writeTreeToFile(wordLength, table.top(), &ch, &chFill);
//    if (chFill > 0) {
//        ch <<= (8 - chFill - 1);
//        of->put(ch);
////        print_char_to_binary(ch);
//    }
    
    root = table.top();
    
    HuffCode code;
    buildMap(root, &tableMap, code);

    analyze(filename, wordLength, &encd, NULL);
    
    // If there are some bits left unencoded,
    // extending it with 0 bits and writing to file
    if (encodedSize != 0) {
        int shift = 8 - encodedSize - 1;
        fakeBitsLength = shift;
        encoded <<= shift;
        of->put(encoded);
    }
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

void vl_decompress(char* filename, int wordLength)
{
    ifstream file (filename , ifstream::in|ifstream::binary);

    // Getting file length
    file.seekg (0, file.end);
    long leftReadBits = file.tellg() * 8;
    file.seekg (0, file.beg);
    
    char* readBuffer = new char[256];
    
    HuffEntry *it = root;
    
    char decoded = 0;
    int decodedBits = 0;
    
    // Building leaves
    while (file.good()) {
        
        size_t length = file.read(readBuffer, 256).gcount();
        
        // Each char of buffer
        int i;
        for (i = 0; i < length; i++) {
            char c = readBuffer[i];
            
            // Each bit of read byte
            int j;
            for (j = (sizeof(c) * 8 - 1); j >= 0; j--) {
                // Last buffer, last char, left only fake bits
                // Extending decoded bit with bits which were left during encoding
                if (fakeBitsLength && (leftReadBits == fakeBitsLength)) {
                    
                    break;
                }
                leftReadBits--;
                
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
                        decoded |= codeBit;
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

                    // Resetting
                    it = root;
                }
                
            }
        }
        
    }
    
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
}

/**
 * handles with arguments.
 */
int main(int argc,char**argv){
    
    if(argc==4 && argv[1][0]=='-')
    {
        if(argv[1][1]=='e')
        {
            int wordLength = 13;
            
            of = new ofstream("compressed.txt", ofstream::out|ofstream::binary);
            vl_encode("test.pdf", "compressed.txt", wordLength);
            of->close();
            
            cout << "reading compressed" << endl;
            
            in = new ofstream("test_d.pdf", ofstream::out|ofstream::binary);
            vl_decompress("compressed.txt", wordLength);
            in->close();
            
        }
        else if(argv[1][1]=='d')
        {
            
        }
        else
        {
            cout<<"Invalid arguments!"<<endl;
            return 1;
        }
        
    }else{
        cout<<"Invalid arguments!"<<endl;
        return 1;
    }
    return 0;
}
