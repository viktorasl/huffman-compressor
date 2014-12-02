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
using namespace std;

/*
 * Node structure for Huffman tree
 *
 * */
struct node
{
    node(node * lft=0,node * rght=0,char ASCI=0,unsigned int frq=0):code()
    {
        left=lft; right=rght; ASCII=ASCI; freq=frq;
    }
    node(const node& other)
    {
        left=other.left; right=other.right; ASCII=other.ASCII; freq=other.freq; code=other.code;
    }
    node * left;
    node * right;
    char   ASCII;
    unsigned int freq;
    string code;
};


/**
 * comparison class to make min heap
 */
struct comp
{
    bool operator()(node* const frst,node* const scnd)
    {
        return frst->freq>scnd->freq;//to make min heap
    }
};
/**
 * comparison class for set
 */
struct comp2
{
    bool operator()(node*  frst,node* scnd)
    {
        return (unsigned int)(frst->ASCII)<(unsigned int)(scnd->ASCII);
    }
};

/**
 * converts single char to 8 bit
 */
string ascii2bin(const char&c)
{
    string bin;
    for(unsigned int y = 0; y < sizeof(char) * 8; y++)
        bin.insert(bin.begin(), ( c & (1 << y) ) ? '1' : '0');
    return bin;
}

/**
 * reads char stream to bit stream using previous method
 */
void bin2str(ifstream & ifs,deque<char> & ss)
{
    char aux = 0;
    while(ifs.good())
    {
        aux=ifs.get(); if(!ifs.good()) break;
        string  tmp = ascii2bin(aux);
        ss.insert(ss.end(),tmp.data(),tmp.data()+tmp.length());
    }
}
//converts 8 bit to char
char bin2ascii(const char*c)
{
    char ascii=0;
    for(unsigned int y = 0; y <(sizeof(char) * 8)-1; y++)
        ascii=(ascii|(c[y]-0x30))<<1;
    return (ascii|(c[7]-0x30));
}

//write bit stream to file
void str2bin(deque<char> & ss,ofstream & of)
{
    while(ss.size()>0)
    {
        of<<bin2ascii(&ss[0]);
        ss.erase(ss.begin(),ss.begin()+8);
    }
}

//write Huffman tree to file, also make encoder mapping
void print(node*cur, map<char,string> &encoder,deque<char> & ss)
{
    if(cur->left==0 )//only left and right links of leaf may be NULL
    {
        ss.push_back('1');
        string  tmp = ascii2bin(cur->ASCII);
        ss.insert(ss.end(),tmp.data(),tmp.data()+tmp.length());
        //cout<<cur->ASCII<<" "<<cur->freq<<endl;
        encoder[cur->ASCII]=cur->code;
    }
    else
    {
        ss.push_back('0');
        cur->left->code.append(cur->code);
        cur->left->code.append("0");
        cur->right->code.append(cur->code);
        cur->right->code.append("1");
        print(cur->left,encoder,ss);
        print(cur->right,encoder,ss);
    }
}

/**
 * converse of printing tree
 */
void read(node*cur,deque<char> & ss)
{
    char aux=ss.front(); ss.erase(ss.begin());
    if(aux=='1' )
    {
        string tmp(ss.begin(),ss.begin()+8);
        cur->ASCII=bin2ascii(&tmp[0]);
        ss.erase(ss.begin(),ss.begin()+8);
    }
    else
    {
        cur->left=new node(0,0,0,0);
        cur->right=new node(0,0,0,0);
        if(cur->left==0||cur->right==0){ cout<<"memory allocation error!"<<endl; exit(1);}
        cur->left->code.append(cur->code);
        cur->left->code.append("0");
        cur->right->code.append(cur->code);
        cur->right->code.append("1");
        read(cur->left,ss);
        read(cur->right,ss);
    }
}

/*
 * decode bit stream to bytes.
 * recursively traverse tree until leaf.
 */
void read(node*cur,deque<char> & ss,ofstream&of)
{
    if(cur->left==0  )
    {
        of<<cur->ASCII;
    }
    else
    {
        char aux=ss.front(); ss.erase(ss.begin());
        if(aux=='0')
            read(cur->left,ss,of);
        else
            read(cur->right,ss,of);
    }
}


void encoder(char*filename,char*outname)
{
    ifstream ifs ( filename , ifstream::in|ifstream::binary ); // open file
    if(!ifs.is_open()){ cout<<"file open error!"<<endl;  exit(1);} // checking if opened
    set<node*,comp2> nodeset;//to find frequencies
    vector<node*> lst; //holds heap
    while (ifs.good())//read file and puts to set
    {
        char aux = ifs.get();  if(!ifs.good()) break;
        node* cand = new node(0,0,aux,1);
        if(cand==0){ cout<<"memory allocation error!"<<endl; exit(1);}
        pair<set<node*>::iterator,bool> ret=nodeset.insert(cand);
        if(!ret.second){
            delete cand;
            (*(ret.first))->freq++;
        }
    }
    for(set<node*>::iterator itr = nodeset.begin();itr!=nodeset.end();itr++)
    {//to use heap push set elements to vector
        lst.push_back(*itr);
    }
    nodeset.clear();
    make_heap(lst.begin(),lst.end(),comp());
    while(lst.size()>1)//pops two least element from heap and push new merged
    {                  //of these nodes
        pop_heap(lst.begin(),lst.end(),comp());
        node* frst = lst.back(); lst.pop_back();
        pop_heap(lst.begin(),lst.end(),comp());
        node* second = lst.back(); lst.pop_back();
        node* parent = new node(frst,second,0,frst->freq+second->freq);
        if(parent==0){ cout<<"memory allocation error!"<<endl; exit(1);}
        lst.push_back(parent);
        push_heap(lst.begin(),lst.end(),comp());
    }
    deque<char> buffer;//buffer to hold bit stream
    map<char,string> encoder;//huffman tree encodings
    print(lst.front(),encoder,buffer);//write tree to file and set mapping
    
    ifs.close();
    ifs.open(filename , ifstream::in|ifstream::binary);
    if(!ifs.is_open()){ cout<<"file open error!"<<endl;  exit(1);}
    while (ifs.good())
    {
        char aux = ifs.get();  if(!ifs.good()) break;
        string & tmp = encoder[aux];
        buffer.insert(buffer.end(),tmp.data(),tmp.data()+tmp.length());
    }
    ofstream ofs ( outname , ofstream::out|ifstream::binary );
    if(!ofs.is_open()){ cout<<"file write error!"<<endl;  exit(1);}
    unsigned int length = buffer.size();
    unsigned int trimmedsize = (length/8)*8;
    unsigned int padding = length-trimmedsize;
    if(padding!=0) padding = 8-padding;
    for(;padding>0;padding--)  buffer.push_back('0');//padding
    ofs.write((const char*) &length, sizeof(unsigned int));
    str2bin(buffer,ofs);//write file
    ofs.close();
}


void decoder(char*filename,char*outname)
{
    node* root = new node(0,0,0,0);//huffman tree root
    if(root==0){ cout<<"memory allocation error!"<<endl; exit(1);}
    deque<char> buffer; //buffer of '1's and '0's,
    ifstream ifs ( filename , ifstream::in|ifstream::binary );
    if(!ifs.is_open()){ cout<<"file open error!"<<endl;  exit(1);}
    unsigned int size; //compressed size
    ifs.read((char*)& size, sizeof(unsigned int));
    unsigned int trimmedsize = (size/8)*8;
    unsigned int padding = size-trimmedsize;
    if(padding!=0) padding = 8-padding;
    bin2str(ifs,buffer);//read all file to buffer converting to bits
    
    for(;padding>0;padding--)  buffer.erase(buffer.end());//read padded 0's
    
    ofstream ofs ( outname , ofstream::out|ofstream::binary );
    if(!ofs.is_open()){ cout<<"file write error!"<<endl;  exit(1);}
    
    read(root,buffer);//read tree
    
    while(buffer.size()>0)//decode and write file as readed from tree.
        read(root,buffer,ofs);
}

void print_char_to_binary(char ch)
{
    int i;
    for (i=(sizeof(ch) * 8 - 1);i>=0;i--)
        printf("%d",((ch & (1<<i))>>i));
    printf("\n");
}

typedef vector<bool> HuffWord;
typedef vector<HuffWord *> HuffWords;

void vl_encode(char*filename, char*outname, int wl)
{
    HuffWords words;
    
    int bufferSize = 5; // buffer size in bytes
    int wordLength = 7; // word length in bits
    
    HuffWord* word = new HuffWord();

    ifstream file (filename , ifstream::in|ifstream::binary);
    char* readBuffer = new char[bufferSize];
    while  (file.good()) {
        
        size_t length = file.read(readBuffer, bufferSize).gcount();
        cout << "Buffer: " << length << " " << readBuffer << endl;
        
        // Each character
        int i;
        for (i = 0; i < length; i++) {
            char ch = readBuffer[i];
            
            cout << "Char: " << ch << " ";
            print_char_to_binary(ch);
            
            // Each characters' bit
            int j;
            for (j = sizeof(char)*8 - 1; j >= 0; j--) {
                bool a = (ch & (1 << j)) >> j;
                word->push_back(a);
                
                // new bitset
                if (word->size() == wordLength) {
                    words.push_back(word);
                    word = new HuffWord();
                }
            }
        }
    }
    
    // Printing all words
    int i;
    for (i = 0; i < words.size(); i++) {
        int j;
        for (j = 0; j < (words[i])->size(); j++) {
            cout << (int)((words[i])->at(j));
        }
        cout << endl;
    }
    
    // There are some bits left
    if (word->size() > 0) {
        int j;
        for (j = 0; j < word->size(); j++) {
            cout << (int)(word->at(j));
        }
        cout << endl;
    }
    
    char* writeBuffer = new char[bufferSize];
    int filledBuffer = 0; // filled buffer size in bytes
    
    char c = 0b00000000;
    int bits = 0;
    
    // Each word
    for (i = 0; i < words.size(); i++) {
        int j;
        
        // Each bit
        for (j = 0; j < (words[i])->size(); j++) {
            c |= words[i]->at(j);
            bits++;
            
            // If end of char, print it and reset bits
            if (bits == 8) {
                cout << c;
                c = 0;
                bits = 0;
            } else {
                c <<= 1;
            }
        }
    }
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
