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

int Transcoder::prepareVideoEncoder(StreamContext *streamContext, AVCodecContext *decoderContext, AVRational &inputFrameRate, StreamParams &streamParams){
    /**
        Prepares a video encoder. The method creates a stream, AVCodec, and AVCodecContext.
        These are kept in the input StreamContext.
        The resulting avCodec will copy video height, width, sample aspect ratio, and sometimes pixel format
        from the input AVCodecContext.
        @param streamContext: A StreamContext that will contain the encoding data
        @param decoderContext: The input AVCodecContext
        @param inputFramerate: the framerate of the input file
        @param streamParams: a StreamParams object containing codec settings
     
     */
    // create a stream
    streamContext->videoAVStream = avformat_new_stream(streamContext->avFormatContext, NULL);
    // setup encoder
    streamContext->videoAVCodec = avcodec_find_encoder_by_name(streamParams.videoCodec.c_str());
    if(!streamContext->videoAVCodec) {
        std::cout << "could not find the proper codec! \n";
        return -1;
    }
    
    // setup codec context for video
    streamContext->videoAVCodecContext = avcodec_alloc_context3(streamContext->videoAVCodec);
    if(!streamContext->videoAVCodecContext) {
        std::cout << "could not allocate memory for codec context! \n";
        return -1;
    }
    
    av_opt_set(streamContext->videoAVCodecContext->priv_data, "preset", "fast", 0); // TODO: make this configurable
    if(streamParams.codecPrivKey.c_str() && streamParams.codecPrivValue.c_str()) {
        av_opt_set(streamContext->videoAVCodecContext->priv_data, streamParams.codecPrivKey.c_str(), streamParams.codecPrivValue.c_str(), 0);
    }
    
    // copy height, width, aspect ratio
    streamContext->videoAVCodecContext->height = decoderContext->height;
    streamContext->videoAVCodecContext->width = decoderContext->width;
    streamContext->videoAVCodecContext->sample_aspect_ratio = decoderContext->sample_aspect_ratio;
    
    // copy pixel format. TODO: make this configurable by user!
    if(streamContext->videoAVCodec->pix_fmts) {
        streamContext->videoAVCodecContext->pix_fmt = streamContext->videoAVCodec->pix_fmts[0];
    } else {
        streamContext->videoAVCodecContext->pix_fmt = decoderContext->pix_fmt;
    }
    
    streamContext->videoAVCodecContext->bit_rate = 3 * 1000 * 1000; // Default to 3Mbit/s
    streamContext->videoAVCodecContext->rc_buffer_size = 6 * 1000 * 1000 + 2 * 100 * 1000;
    streamContext->videoAVCodecContext->rc_max_rate = 4.7 * 1000 * 1000;
    streamContext->videoAVCodecContext->rc_min_rate = 3 * 1000 * 1000;
    
    // setup time base (use input frame rate for this)
    streamContext->videoAVCodecContext->time_base = av_inv_q(inputFrameRate);
    streamContext->videoAVStream->time_base = streamContext->videoAVCodecContext->time_base;
    
    if(avcodec_open2(streamContext->videoAVCodecContext, streamContext->videoAVCodec, NULL) < 0) {
        std::cout <<  "could not open the codec! \n";
        return -1;
    }
    avcodec_parameters_from_context(streamContext->videoAVStream->codecpar, streamContext->videoAVCodecContext);
    return 0;
}

int Transcoder::prepareAudioEncoder(StreamContext *streamContext, int &sampleRate, StreamParams &streamParams){
    streamContext->audioAVStream = avformat_new_stream(streamContext->avFormatContext, NULL);
    
    streamContext->audioAVCodec = avcodec_find_encoder_by_name(streamParams.audioCodec.c_str());
    if(!streamContext->audioAVCodec) {
        std::cout << "Could not find the correct audio codec! \n";
        return -1;
    }
    streamContext->audioAVCodecContext = avcodec_alloc_context3(streamContext->audioAVCodec);
    if(!streamContext->audioAVCodecContext) {
        std::cout << "could not allocate memory for audio codec context! \n";
        return -1;
    }
    
    int OUTPUT_CHANNELS = 2; // only stereo audio for now, maybe we add configurable channels down the line ?
    int OUTPUT_BIT_RATE = 196000; // default to 196kbps, make this configurable later!
    
    // configure our codec context
    streamContext->audioAVCodecContext->channels = OUTPUT_CHANNELS;
    streamContext->audioAVCodecContext->channel_layout = av_get_default_channel_layout(OUTPUT_CHANNELS);
    streamContext->audioAVCodecContext->sample_rate = sampleRate;
    streamContext->audioAVCodecContext->sample_fmt = streamContext->audioAVCodec->sample_fmts[0];
    streamContext->audioAVCodecContext->bit_rate = OUTPUT_BIT_RATE;
    streamContext->audioAVCodecContext->time_base = (AVRational){1, sampleRate};
    
    streamContext->audioAVCodecContext->strict_std_compliance = FF_COMPLIANCE_EXPERIMENTAL;
    
    streamContext->audioAVStream->time_base = streamContext->audioAVCodecContext->time_base; //match time bases
    
    if(avcodec_open2(streamContext->audioAVCodecContext, streamContext->audioAVCodec, NULL) < 0) {
        std::cout << "Could not open the codec! \n";
        return -1;
    }
    
    // set the codec parameters based on our context params
    avcodec_parameters_from_context(streamContext->audioAVStream->codecpar, streamContext->audioAVCodecContext);
    
    return 0;
}

int Transcoder::remux(AVPacket **packet, AVFormatContext **formatContext, AVRational decoderTb, AVRational encoderTb){
    /**
        Remuxes a packet
     */
    av_packet_rescale_ts(*packet,decoderTb, encoderTb);
    if(av_interleaved_write_frame(*formatContext, *packet) < 0) {
        std::cout << "Failed to copy frame! \n";
        return -1;
    }
    return 0;
}

int Transcoder::prepareCopy(AVFormatContext *avFormatContext, AVStream **avStream, AVCodecParameters *decoderParameters) {
    /**
        Bootstraps settings for copying a stream
     */
    *avStream = avformat_new_stream(avFormatContext, NULL);
    avcodec_parameters_copy((*avStream)->codecpar, decoderParameters);
    return 0;
}

int Transcoder::encodeVideo(StreamContext *decoderContext, StreamContext *encoderContext, AVFrame *inputFrame) {
    /**
         Encodes a video  AVFrame to the encoder StreamContext.
         Copies the stream index and time base from the decoder StreamContext
         @param decoderContext: StreamContext for the decoder (i.e input)
         @param encoderContext: StreamContext for the encoder (i.e output)
         @param inputFrame: The frame to encode
         @returns 0 if succesful, -1 otherwise
     */
    if(inputFrame) inputFrame->pict_type = AV_PICTURE_TYPE_NONE; //reset frame type to let the encoder do whatever
    // allocate memory for output packet
    AVPacket *outPacket = av_packet_alloc();
    if(!outPacket) {
        std::cout << "could not allocate memory for output packet!! \n";
        return -1;
    }
    
    // send raw video frame to encoder
    int response = avcodec_send_frame(encoderContext->videoAVCodecContext, inputFrame);
    // response will be 0 as long as everything is OK, we use this to loop
    while (response >= 0) {
        // receive the encoded packet
        response = avcodec_receive_packet(encoderContext->videoAVCodecContext, outPacket);
        if(response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            // we're done with the file, exit loop
            break;
        }
        else if (response < 0) {
            // some other error occured
            std::cout << "Error when receiving packet from encoder! \n" << av_err2str(response) << "\n";
            return -1;
        }
        
        // set time base and duration
        outPacket->stream_index = decoderContext->videoIndex;
        outPacket->duration = encoderContext->videoAVStream->time_base.den / encoderContext->videoAVStream->time_base.num / decoderContext->videoAVStream->avg_frame_rate.num * decoderContext->videoAVStream->avg_frame_rate.den;
        // convert to output time base
        av_packet_rescale_ts(outPacket, decoderContext->videoAVStream->time_base, encoderContext->videoAVStream->time_base);
        response = av_interleaved_write_frame(encoderContext->avFormatContext, outPacket);
        if (response != 0) {
            std::cout << "Error " << response << " when receiving packet from decoder! " << av_err2str(response) << "\n";
            return -1;
        }
       
    }
    // deref and free
    av_packet_unref(outPacket);
    av_packet_free(&outPacket);
    
    return 0;
}

int Transcoder::transcodeVideo(StreamContext *decoderContext, StreamContext *encoderContext, AVPacket *inputPacket, AVFrame *inputFrame) {
    // send the raw data to the decoder
    int response = avcodec_send_packet(decoderContext->videoAVCodecContext, inputPacket);
    if (response < 0) {
        std::cout << "Error while sending packet to decoder! \n";
        return response;
    }
    while(response >= 0) {
        // read the decoded frame
        response = avcodec_receive_frame(decoderContext->videoAVCodecContext, inputFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            // no more to read,end loop
            break;
        }
        else if (response < 0) {
            std::cout << "Error " << response << " when receiving frame from decoder " << av_err2str(response);
            return response;
        }
        
        if (response >= 0) {
            // frame was read correctly, encode it
            if(encodeVideo(decoderContext, encoderContext, inputFrame) < 0) return -1;
        }
        // unref the frame
        av_frame_unref(inputFrame);
    }
    return 0;
}

int Transcoder::transcodeAudio(StreamContext *decoderContext, StreamContext *encoderContext, AVPacket *inputPacket, AVFrame *inputFrame) {
    int response = avcodec_send_packet(decoderContext->audioAVCodecContext, inputPacket);
    if (response < 0 ) {
        std::cout << "Error while sending packet to decoder! \n";
        return response;
    }
    while(response >= 0) {
        // read the decoded frame(s)
        response = avcodec_receive_frame(decoderContext->audioAVCodecContext, inputFrame);
        if (response == AVERROR(EAGAIN) || response == AVERROR_EOF) {
            // no more to read,end loop
            break;
        }
        else if (response < 0) {
            std::cout << "Error " << response << " when receiving frame from decoder " << av_err2str(response);
            return response;
        }
        if (response >= 0) {
            if (encodeAudio(decoderContext,encoderContext,inputFrame) < 0) return -1;
        }
        
        av_frame_unref(inputFrame);
    }
    return 0;

}

int Transcoder::encodeAudio(StreamContext *decoderContext, StreamContext *encoderContext, AVFrame *inputFrame){
    /**
        Encodes an audio AVFrame to the encoder StreamContext.
        Copies the stream index and time base from the decoder StreamContext
        @param decoderContext: StreamContext for the decoder (i.e input)
        @param encoderContext: StreamContext for the encoder (i.e output)
        @param inputFrame: The frame to encode
        @returns 0 if succesful, -1 otherwise
     */
    AVPacket *outPacket = av_packet_alloc();
    if(!outPacket) {
        std::cout << "could not allocate memory for output packet!! \n";
        return -1;
    }
    
    int response = avcodec_send_frame(encoderContext->audioAVCodecContext, inputFrame);
    while(response >= 0) {
        response = avcodec_receive_packet(encoderContext->audioAVCodecContext, outPacket);
        if(response == AVERROR(EAGAIN) || response == AVERROR_EOF){
            break;
        } else if (response != 0) {
            std::cout << "Error " << response << " when receiving packet from decoder! " << av_err2str(response) << "\n";
            return -1;
        }
        outPacket->stream_index = decoderContext->audioIndex;
        av_packet_rescale_ts(outPacket, decoderContext->audioAVCodecContext->time_base, encoderContext->audioAVCodecContext->time_base);
        response = av_interleaved_write_frame(encoderContext->avFormatContext, outPacket);
        if (response != 0){
            std::cout << "Error " << response << " while receiving packet from decoder: " << av_err2str(response) << "\n";
            return -1;
        }
    }
    // unref and free memory
    av_packet_unref(outPacket);
    av_packet_free(&outPacket);
    return 0;
    
}



int Transcoder::Transcode(std::string &inputFile, std::string &outputFile, std::string &codec, std::string &codecPrivkey, std::string &codecPrivValue, bool copyAudio, bool copyVideo) {
    /**
        Transcodes a video file and writes the result to an output file.
        @param inputFile: the URL of the file to transcode (i.e input file)
        @param outputFile: the URL of the output file (the transcoded file)
        @param codec: the name of codec library (for example: "libx264")
        @param codecPrivKey: the key for the codec settings (example: "x264-params")
        @param codecPrivValue: the codec settings (example: "keyint=123:min-keyint:23")
        @param copyAudio: if true, the audio is simply transmuxed and not transcoded
        @param copyVideo: same as copyAudio, but for the video stream
     */
    
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
    encoder->fileName = outputFile;
    
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
        AVRational inputFrameRate = av_guess_frame_rate(decoder->avFormatContext, decoder->videoAVStream, NULL);
        prepareVideoEncoder(encoder, decoder->videoAVCodecContext, inputFrameRate,streamParams);
    }
    else {
        if(prepareCopy(encoder->avFormatContext, &encoder->videoAVStream, decoder->videoAVStream->codecpar) < 0) { //try to prepare for copying
            return -1;
        }
    }
    
    if(!streamParams.copyAudio) {
        // TODO: set up audio encoder
        if(prepareAudioEncoder(encoder, decoder->audioAVCodecContext->sample_rate, streamParams) < 0) {
            return -1;
        }
    } else {
        //TODO: set up copy
        if(prepareCopy(encoder->avFormatContext, &encoder->audioAVStream, decoder->audioAVStream->codecpar)) {
            return -1;
        }
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
        // I cant find a way to hot-swap in C++, so we'll do it the ugly way
        if(decoder->avFormatContext->streams[inPacket->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO){
            if(!streamParams.copyVideo) {
                if (transcodeVideo(decoder, encoder, inPacket, inFrame) < 0) {
                    return -1;
                }
                av_packet_unref(inPacket);
            } else {
                if(remux(&inPacket, &encoder->avFormatContext, decoder->videoAVStream->time_base, encoder->videoAVStream->time_base) < 0) {
                    return -1;
                }
            }
        } else if (decoder->avFormatContext->streams[inPacket->stream_index]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO){
            if(!streamParams.copyAudio) {
                if (transcodeAudio(decoder, encoder, inPacket, inFrame) < 0) {
                    return -1;
                }
                av_packet_unref(inPacket);
            } else {
                if(remux(&inPacket, &encoder->avFormatContext, decoder->videoAVStream->time_base, encoder->videoAVStream->time_base) < 0) {
                    return -1;
                }
            }
        } else {
            std::cout << "ignoring non video/audio packages \n";
        }
    }
    
    // flush video encoder
    if(encodeVideo(decoder, encoder, NULL) < 0) {
        return -1;
    }
    
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
