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

using namespace std;

typedef vector<bool> HuffCode;
typedef map<int, HuffCode> HuffTable;


#ifdef DEBUG
void print_char_to_binary(char ch)
{
    int i;
    for (i=(sizeof(ch) * 8 - 1);i>=0;i--)
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
        cout << "value=" << root->value << " freq=" << root->frequency << " code=" << code << endl;
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
                
                wordInt |= bit;
                wordFill++;
                
                // Finished composing word of wordLength bits, making HuffEntry leaf
                if (wordFill == wordLength) {
                    //                    cout << "Code: " << wordInt << endl;
                    
                    callback(wordFill, wordInt);
                    
                    // Resetting
                    wordInt = 0;
                    wordFill = 0;
                    
                } else {
                    // Shifting value
                    wordInt <<= 1;
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
    cout << size;
    // Increse frequency if value already exist
    int k;
    bool exists = false;
    for (k = 0; k < entries.size(); k++) {
        if (entries[k]->value == word) {
            entries[k]->frequency++;
            exists = true;
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
//        cout << cd[i];
        encoded |= cd[i];
        encodedSize++;
        
        //11001010000001111101111001100101000000111110111101110011 10110
        //11001010000001111101111001100101000000111110111101110011
        //11001010000001111101111001100101000000111110111101110011 10110000
        if (encodedSize == 8) {
            print_char_to_binary(encoded);
            of->put(encoded);
            encoded = 0;
            encodedSize = 0;
        } else {
            encoded <<= 1;
        }
        
    }
}

void vl_encode(char*filename, char*outname, int wl)
{
    int wordLength = 16; // word length in bits
    
    analyze(filename, wordLength, &gather, NULL);
    
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
    // If there are some bits left unencoded
    if (encodedSize != 0) {
        int shift = 8 - encodedSize - 1;
        encoded <<= shift;
        print_char_to_binary(encoded);
        of->put(encoded);
    }
}

void vl_decompress(char* filename)
{
    ifstream file (filename , ifstream::in|ifstream::binary);
    char* readBuffer = new char[256];
    
    HuffEntry *it = root;
    
    // Building leaves
    while (file.good()) {
        
        size_t length = file.read(readBuffer, 256).gcount();
        
        // Each char
        int i;
        for (i = 0; i < length; i++) {
            char c = readBuffer[i];
            
            // Each bit
            int j;
            for (j = (sizeof(c) * 8 - 1); j >= 0; j--) {
                
                if (!it->left && !it->right) {
                    cout << it->value;
                    it = root;
                }
                
                bool bit = (c & (1 << j)) >> j;
                if (bit == 0) {
                    it = it->left;
                } else {
                    it = it->right;
                }
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
            of = new ofstream("compressed.txt", ofstream::out|ofstream::binary);
            vl_encode(argv[2], argv[3], 8);
            of->close();
            
            cout << "reading compressed" << endl;
            
            vl_decompress("compressed.txt");
            
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
