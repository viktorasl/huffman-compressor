//
//  HuffEncoder.cpp
//  huffman
//
//  Created by Viktoras Laukevičius on 05/12/14.
//  Copyright (c) 2014 viktorasl. All rights reserved.
//

#include "HuffCompressor.h"
#include <fstream>
#include <sstream>

using namespace std;

HuffCompressor::HuffCompressor()
{
    
}

HuffCompressor::~HuffCompressor()
{
    if (this->compressed && this->compressed->is_open()) {
        this->compressed->close();
        delete this->compressed;
    }
    if (this->file && this->file->is_open()) {
        this->file->close();
        delete this->file;
    }
}

void HuffCompressor::compress(string filename, int wordLength)
{
    if (wordLength < 0x2 || wordLength > 0x10) {
        string err = "Word length must be between 2 and 16";
        throw err;
    }
    
    // Opening file that should be compresssed
    this->file = new ifstream(filename, ifstream::in|ifstream::binary);
    if (! this->file->is_open()) {
        stringstream ss;
        ss << "Could not open file named " << filename;
        throw ss.str();
    }
    
    this->compressed = new ofstream("compressed.cd", ofstream::out|ofstream::binary);
}