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
    /**
        Fills a stream with correct information
        @param avStream an AVStream containing stream information
        @param avCodec an AVCodec to fill
        @param avCodecContext an AVCodecContext to fill
        @returns 0 if successful, -1 if an error occured
     */
    // first, we find the decoder
    *avCodec = avcodec_find_decoder(avStream->codecpar->codec_id);
    if(!avCodec) {
        std::cout << "Failed to find the codec! \n";
        return -1;
    }
    // allocate the codec context
    *avCodecContext = avcodec_alloc_context3(*avCodec);
    if(!avCodecContext) {
        std::cout << "Failed to allocate memory for the codec context! \n";
        return -1;
    }
    
    if(avcodec_parameters_to_context(*avCodecContext, avStream->codecpar) < 0){
        std::cout << "failed to fill the codec context! \n";
        return -1;
    }
    if(avcodec_open2(*avCodecContext, *avCodec, NULL) < 0) {
        std::cout << "failed to open codec! \n";
        return -1;
    }
    return 0;
}

int Transcoder::prepareDecoder(StreamContext *sc) {
    /**
            Prepares the decoder for an AVFormatContext
            @param sc a StreamContext object
            @returns 0 if successful, -1 if error occured
     */
    // iterate over streams, we only keep audio and video in this case
    for(int i = 0; i < sc->avFormatContext->nb_streams; i++){
        if(sc->avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            sc->videoAVStream = sc->avFormatContext->streams[i];
            sc->videoIndex = i;
            
            if(fillStreamInfo(sc->videoAVStream, &sc->videoAVCodec, &sc->videoAVCodecContext) < 0) {
                return -1;
            }
        }
        else if(sc->avFormatContext->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            sc->audioAVStream = sc->avFormatContext->streams[i];
            
            if(fillStreamInfo(sc->audioAVStream, &sc->audioAVCodec, &sc->audioAVCodecContext) <0) {
                return -1;
            }
        }
        else {
            std::cout << "Stream " << i << " is neither audio nor video, skipping \n";
        }
    }
    return 0;
}

int Transcoder::Transcode(std::string &inputFile, std::string &outputFile, std::string &codec, std::string &codecPrivkey, std::string &codecPrivValue, bool copyAudio, bool copyVideo) {
    
    // init our streamparams
    // maybe pass this as an argument instead, and init somewhere else?
    StreamParams streamParams = {};
    streamParams.copyAudio = copyAudio;
    streamParams.copyVideo = copyVideo;
    streamParams.videoCodec = codec;
    streamParams.codecPrivKey = codecPrivkey;
    streamParams.codecPrivValue = codecPrivValue;
    
    // init StreamContexts for encoder and decoder
    StreamContext *decoder = (StreamContext*) calloc(1, sizeof(StreamContext));
    decoder->fileName = inputFile;
    
    StreamContext *encoder = (StreamContext*) calloc(1, sizeof(StreamContext));
    
    if(openMedia(decoder->fileName, &decoder->avFormatContext) < 0) return -1;
    if(prepareDecoder(decoder) <0 ) return  -1;
    
    // alloc output context for our new file
    avformat_alloc_output_context2(&encoder->avFormatContext, NULL, NULL, encoder->fileName.c_str());
    // check that context was created correctly
    if(!encoder->avFormatContext) {
        std::cout << "Could not allocate memory for the output format! \n";
        return -1;
    }
    
    if(!streamParams.copyVideo) {
        // TODO: set up video encoder
    }
    else {
        // TODO: set up copy
    }
    
    if(!streamParams.copyAudio) {
        // TODO: set up audio encoder
    } else {
        //TODO: set up copy
    }
    
    if(encoder->avFormatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        encoder->avFormatContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    if(!(encoder->avFormatContext->oformat->flags & AVFMT_NOFILE)){
        if(avio_open(&encoder->avFormatContext->pb, encoder->fileName.c_str(), AVIO_FLAG_WRITE ) < 0){
            std::cout << "could not open the output file! \n";
            return -1;
        }
    }
    
    AVDictionary* muxerOptions = NULL;
    // we use c_str() for easier evaluation
    if(streamParams.muxerOptKey.c_str() && streamParams.muxerOptValue.c_str()){
        av_dict_set(&muxerOptions, streamParams.muxerOptKey.c_str(),streamParams.muxerOptValue.c_str(), 0);
    }
    
    if(avformat_write_header(encoder->avFormatContext, &muxerOptions) < 0) {
        std::cout << "An error occured when opening the output file! \n";
        return -1;
    }
    
    // allocate memory for frames and packets
    AVFrame *inFrame = av_frame_alloc();
    if(!inFrame) {
        std::cout << "Failed to allocate memory for AVFrame";
        return -1;
    }
    
    AVPacket *inPacket = av_packet_alloc();
    if(!inPacket){
        std::cout << "Failed to allocate memory for AVPacket";
        return -1;
    }
    // read the input file. av_read_frame returns zero if OK,
    // < 0 if an error occured or it has reached EOF.
    while(av_read_frame(decoder->avFormatContext, inPacket) >= 0) {
        // TODO: set up transcoding or muxing here!
        // use a function pointer to separate audio/video coding?
    }
    
    // TODO: flush video encoder
    
    av_write_trailer(encoder->avFormatContext);
    
    // free memory
    if(muxerOptions != NULL) {
        av_dict_free(&muxerOptions);
        muxerOptions = NULL;
    }
    
    if(inFrame != NULL) {
        av_frame_free(&inFrame);
        inFrame = NULL;
    }
    
    if(inPacket != NULL) {
        av_packet_free(&inPacket);
        inPacket = NULL;
    }
    
    avformat_close_input(&decoder->avFormatContext);
    avformat_free_context(decoder->avFormatContext); decoder->avFormatContext = NULL;
    avformat_free_context(encoder->avFormatContext); encoder->avFormatContext = NULL;
    avcodec_free_context(&decoder->videoAVCodecContext); decoder->videoAVCodecContext = NULL;
    avcodec_free_context(&decoder->audioAVCodecContext); decoder->audioAVCodecContext = NULL;
    free(decoder);
    decoder = NULL;
    free(encoder);
    encoder = NULL;
    return 0;
}
