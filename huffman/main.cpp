//
//  main.cpp
//  huffman
//
//  Created by Viktoras Laukevičius on 02/12/14.
//  Copyright (c) 2014 viktorasl. All rights reserved.
//

//============================================================================
// Name        : huff.cpp
// Author      : Ahmet Celik
// Version     : 2009400111
//============================================================================


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

void print_char_to_binary(char ch)
{
    int i;
    for (i=(sizeof(ch) * 8 - 1);i>=0;i--)
        printf("%d",((ch & (1<<i))>>i));
    printf("\n");
}

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

void vl_encode(char*filename, char*outname, int wl)
{
    int bufferSize = 256; // buffer size in bytes
    int wordLength = 4; // word length in bits
    
    vector<HuffEntry *> entries;
    
    int wordInt = 0;
    int wordFill = 0;
    
    ifstream file (filename , ifstream::in|ifstream::binary);
    char* readBuffer = new char[bufferSize];
    while  (file.good()) {
        
        size_t length = file.read(readBuffer, bufferSize).gcount();
        
        // Each character
        int i;
        for (i = 0; i < length; i++) {
            char ch = readBuffer[i];
//            cout << "Char: " << ch << " ";
//            print_char_to_binary(ch);
            
            // Each characters' bit
            int j;
            for (j = sizeof(char) * 8 - 1; j >= 0; j--) {
                bool bit = (ch & (1 << j)) >> j;

                wordInt |= bit;
                wordFill++;
                
                if (wordFill == wordLength) {
//                    cout << "Code: " << wordInt << endl;
                    
                    int k;
                    bool exists = false;
                    for (k = 0; k < entries.size(); k++) {
                        if (entries[k]->value == wordInt) {
                            entries[k]->frequency++;
                            exists = true;
                        }
                    }
                    
                    if (! exists) {
                        HuffEntry *h = new HuffEntry();
                        h->value = wordInt;
                        h->frequency = 1;
                        entries.push_back(h);
                    }
                    
                    wordInt = 0;
                    wordFill = 0;
                    
                } else {
                    wordInt <<= 1;
                }
            }
        }
    }
    
    priority_queue<HuffEntry *, vector<HuffEntry *>, Comp> table;
    
    
    // Printint all entries
    int i;
    for (i = 0; i < entries.size(); i++) {
        table.push(entries[i]);
//        cout << "E: " << entries[i].value << " (" << entries[i].frequency << ")" << endl;
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
        
//        cout << "E: " << a->value << " (" << a->frequency << ")" << endl;
    }
    
    HuffEntry *root = table.top();
    around(root, "");
    
#warning print this vector to file
    
        // There are some bits left
//    if (word->size() > 0) {
//        int j;
//        for (j = 0; j < word->size(); j++) {
//            cout << (int)(word->at(j));
//        }
//        cout << endl;
//    }
}





/**
 * handles with arguments.
 */
int main(int argc,char**argv){
    
    if(argc==4 && argv[1][0]=='-')
    {
        if(argv[1][1]=='e')
        {
            vl_encode(argv[2], argv[3], 8);
//            encoder(argv[2],argv[3]);
        }
        else if(argv[1][1]=='d')
        {
//            decoder(argv[2],argv[3]);
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
