//
//  transcoder.c
//  ffmpeg-experiments
//
//  Created by Foppe on 2021-05-21.
//

#include "transcoder.hpp"
#include <iostream>

int Transcoder::openMedia(const std::string &inputFileName, AVFormatContext **avfc){
    /**
            Method to open the given media file.
            @param inputFileName the URL of the input file
            @param avfc an AVFormatContext for the file
            @returns 0 if succesful, -1 if error occurs
     */
    *avfc = avformat_alloc_context();
    if(!avfc) {
        std::cout << "failed to allocate memory for input format! \n";
        return -1;
    }
    if(avformat_open_input(avfc, inputFileName.c_str(), NULL, NULL) != 0){
        std::cout << "failed to open input file: " << inputFileName << "\n";
        return -1;
    }
    if(avformat_find_stream_info(*avfc, NULL) < 0){
        std::cout << "failed to get stream information \n";
        return -1;
    }
    return 0;
}

int Transcoder::fillStreamInfo(AVStream *avStream, AVCodec **avCodec, AVCodecContext **avCodecContext){
    
}

int Transcoder::prepareDecoder(StreamContext &sc) {
    /**
            Prepares the decoder for an AVFormatContext
            @param sc a StreamContext object
            @returns 0 if successful, -1 if error occured
     */
    // iterate over streams, we only keep audio and video in this case
    for(int i = 0; i < sc.avFormatContext->nb_streams; i++){
        if(sc.avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            sc.videoAVStream = sc.avFormatContext->streams[i];
            sc.videoIndex = i;
        }
        else if(sc.avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            sc.audioAVStream = sc.avFormatContext->streams[i];
        }
        else {
            std::cout << "Stream " << i << " is neither audio nor video, skipping \n";
        }
    }
    return 0;
}
