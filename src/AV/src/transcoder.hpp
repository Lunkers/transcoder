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
    #include <libavutil/opt.h>
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
    int Transcode(std::string &inputFile, std::string &outputFile, std::string &codec, std::string &codecPrivkey, std::string &codecPrivValue,bool copyAudio = true, bool copyVideo = false);
private:
    int openMedia(const std::string &inputFileName, AVFormatContext **avfc);
    int prepareDecoder(StreamContext *sc); // TODO: refactor signature for consistency
    int fillStreamInfo(AVStream *avStream, AVCodec **avCodec, AVCodecContext **avCodecContext);
    int prepareVideoEncoder(StreamContext *streamContext, AVCodecContext *decoderContext, AVRational &inputFrameRate, StreamParams &streamParams);
    int prepareAudioEncoder(StreamContext *streamContext, int &sampleRate, StreamParams &streamParams);
    int prepareCopy(AVFormatContext *avFormatContext, AVStream **avStream, AVCodecParameters *decoderParameters);
    int remux(AVPacket **packet, AVFormatContext *formatContext, AVRational decoderTb, AVRational encoderTb);
    int encodeVideo(StreamContext *decoderContext, StreamContext *encoderContext, AVFrame *inputFrame);
    int encodeAudio(StreamContext *decoderContext, StreamContext *encoderContext, AVFrame *inputFrame);
    int transcodeVideo(StreamContext *decoderContext, StreamContext *encoderContext, AVPacket *inputPacket, AVFrame *inputFrame);
    int transcodeAudio(StreamContext *decoderContext, StreamContext *encoderContext, AVPacket *inputPacket, AVFrame *inputFrame);
    
};

#endif /* transcoder_h */
