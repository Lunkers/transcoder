//
//  transmuxer.hpp
//  ffmpeg-experiments
//
//  Created by Foppe on 2021-05-19.
//
#pragma once
#ifndef transmuxer_hpp
#define transmuxer_hpp
#include <string>
#include <iostream>
#define __STDC_CONSTANT_MACROS
extern "C" {
    #include <libavutil/timestamp.h>
    #include <libavformat/avformat.h>
}

class Transmuxer {
public:
    int transmux (std::string &inputFileName, std::string &outputFileName);
private:
    AVFormatContext* inputFormatContext = NULL;
    AVFormatContext* outputFormatContext = NULL;
    int cleanUp(int* streamList, int &ret);
};

#include <stdio.h>

#endif /* transmuxer_hpp */
