//
//  HuffEncoder.h
//  huffman
//
//  Created by Viktoras Laukevičius on 05/12/14.
//  Copyright (c) 2014 viktorasl. All rights reserved.
//

#ifndef __huffman__HuffCompressor__
#define __huffman__HuffCompressor__

#include <stdio.h>
#include <fstream>

using namespace std;

class HuffCompressor
{
private:
    ofstream *compressed;
    ifstream *file;
public:
    HuffCompressor();
    ~HuffCompressor();
    void compress(string filename, int wordLength);
};

#endif /* defined(__huffman__HuffEncoder__) */
