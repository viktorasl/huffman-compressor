// Encoded file: word_size|left_bits|tree|content

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

#define MIN(X, Y) (((X) < (Y)) ? (X) : (Y))

using namespace std;

typedef vector<bool> HuffCode;
typedef map<int, HuffCode> HuffTable;

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

struct HuffEntry {
    int value = 0;
    int frequency = 0;
    HuffEntry *left = NULL;
    HuffEntry *right = NULL;
};

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

typedef void (*CallbackType)(int, int);

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
            //            cout << "Char: " << ch << " ";
            //            print_char_to_binary(ch);
            
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
                    //                    cout << "Code: " << wordInt << endl;
                    
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

vector<HuffEntry *> entries;

void gather(int size, int word) {
    // Increse frequency if value already exist
    int k;
    bool exists = false;
    for (k = 0; k < entries.size(); k++) {
        if (entries[k]->value == word) {
            entries[k]->frequency++;
            exists = true;
            break;
        }
    }
    
    // Creating new entry
    if (! exists) {
        HuffEntry *h = new HuffEntry();
        h->value = word;
        h->frequency = 1;
        entries.push_back(h);
    }
}

int leftBits;
int leftBitsLength;

void leftBitsHandler(int size, int word)
{
    leftBits = word;
    leftBitsLength = size;
}

HuffTable tableMap;
char encoded;
int encodedSize;
HuffEntry *root;

ofstream* of;

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

int fakeBitsLength;

void vl_encode(char*filename, char*outname, int wordLength)
{
    analyze(filename, wordLength, &gather, &leftBitsHandler);
    
    #warning write this vector to file
    priority_queue<HuffEntry *, vector<HuffEntry *>, Comp> table;
    
    // Making priority queue from vector
    int i;
    for (i = 0; i < entries.size(); i++) {
        table.push(entries[i]);
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
    
    root = table.top();
    
    HuffCode code;
    around(root, "");
    buildMap(root, &tableMap, code);
    
    analyze(filename, wordLength, &encd, NULL);
    
    // If there are some bits left unencoded,
    // extending it with 0 bits and writing to file
    if (encodedSize != 0) {
        int shift = 8 - encodedSize - 1;
        fakeBitsLength = shift;
        encoded <<= shift;
//        print_char_to_binary(encoded);
        of->put(encoded);
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
//                            cout << decoded;
//                            print_char_to_binary(decoded);
                            
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
        //            cout << "Left length: " << leftBitsLength;
        int k;
        for (k = leftBitsLength - 1; k >= 0; k--) {
            bool bit = (leftBits & (1 << k)) >> k;
            
            decoded |= bit;
            decodedBits++;
            
            // Full char
            if (decodedBits == 8) {
                in->put(decoded);
                //                    cout << decoded;
                //                    print_char_to_binary(decoded);
                
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
            int wordLength = 12;
            
            of = new ofstream("compressed.txt", ofstream::out|ofstream::binary);
            vl_encode("test.pdf", "compressed.txt", wordLength);
            of->close();
            
            cout << "reading compressed" << endl;
            
            in = new ofstream("test_dec.pdf", ofstream::out|ofstream::binary);
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
