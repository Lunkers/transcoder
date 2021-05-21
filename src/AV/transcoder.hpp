//
//  transcoder.hpp
//  ffmpeg-experiments
//
//  Created by Foppe on 2021-05-21.
//
#pragma once
#ifndef transcoder_h
#define transcoder_h

#include <string>

#define __STDC_CONSTANT_MACROS
extern "C" {
    #include <libavutil/timestamp.h>
    #include <libavformat/avformat.h>
    #include <libavcodec/avcodec.h>
}
typedef struct StreamParams {
    bool copyVideo;
    bool copyAudio;
    std::string outputExtenstion;
    std::string muxerOptKey;
    std::string muxerOptValue;
    std::string videoCodec;
    std::string audioCodec;
    std::string codecPrivKey;
    std::string codecPrivValue;
} StreamParams;

typedef struct StreamContext {
    AVFormatContext *avFormatContext;
    AVCodec *videoAVCodec;
    AVCodec *audioAVCodec;
    AVStream *videoAVStream;
    AVStream *audioAVStream;
    AVCodecContext *videoAVCodecContext;
    AVCodecContext *audioAVCodecContext;
    int videoIndex;
    int audioIndex;
    std::string fileName;
} StreamContext;

class Transcoder {
public:
    std::string inputCodec;
    std::string outputCodec;
private:
    int openMedia(const std::string &inputFileName, AVFormatContext **avfc);
    int prepareDecoder(StreamContext *sc);
    int fillStreamInfo(AVStream *avStream, AVCodec **avCodec, AVCodecContext **avCodecContext);
};

#endif /* transcoder_h */
