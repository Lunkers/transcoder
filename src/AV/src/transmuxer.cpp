//
//  transmuxer.cpp
//  ffmpeg-experiments
//
//  Created by Fredrik Lundkvist on 2021-05-19.
//

#include "transmuxer.hpp"

int Transmuxer::transmux(std::string &inputFileName, std::string &outputFileName) {
    AVPacket packet;
    
        int ret, i;
        int streamIndex = 0;
        int numStreams = 0;
        int *streamsList = NULL;
        
        // attempt to open the input file
        if(( ret = avformat_open_input(&inputFormatContext, inputFileName.c_str(), NULL, NULL)) < 0) {
            std::cout << "Could not open input file!";
            return cleanUp(streamsList,ret);
        };
        
        // attempt finding stream info in input file
        if((ret = avformat_find_stream_info(inputFormatContext, NULL)) < 0) {
            std::cout << "Failed to retrieve input stream info!";
            return cleanUp(streamsList, ret);
            
        }
        // allocate an AVContext for the output
        avformat_alloc_output_context2(&outputFormatContext, NULL, NULL, outputFileName.c_str());
        if(!outputFormatContext){ //null check for output context
            std::cout << "Failed to allocate memory for output context!";
            ret = AVERROR_UNKNOWN;
            return cleanUp(streamsList, ret);
        }
        
        numStreams = inputFormatContext->nb_streams;
        streamsList = (int*) av_mallocz_array(numStreams, sizeof( *streamsList ));
        
        if(!streamsList) {
            ret = AVERROR(ENOMEM);
            return cleanUp(streamsList,ret);
        }
        
        for(int i = 0; i < inputFormatContext->nb_streams; i++) {
            AVStream *outStream;
            AVStream *inStream = inputFormatContext->streams[i];
            AVCodecParameters *inCodecPar = inStream->codecpar;
            if (inCodecPar->codec_type != AVMEDIA_TYPE_VIDEO &&
                inCodecPar->codec_type != AVMEDIA_TYPE_AUDIO &&
                inCodecPar->codec_type != AVMEDIA_TYPE_SUBTITLE) {
                streamsList[i] = -1; //mark for skipping later
                continue;
            }
            streamsList[i] = streamIndex++;
            outStream = avformat_new_stream(outputFormatContext, NULL);
            if(!outStream) { // null check for out stream
                std::cout << "Failed to allocate memory for output stream!";
                ret = AVERROR_UNKNOWN;
                return cleanUp(streamsList, ret);
                
            }
            
            ret = avcodec_parameters_copy(outStream->codecpar, inCodecPar);
            if (ret < 0 ){
                std::cout << "Could not copy codec parameters!";
                return cleanUp(streamsList, ret);
            }
            
        }
        // print output format info
        av_dump_format(outputFormatContext, 0, outputFileName.c_str(), 1);
        // TODO: Complete method
        
        // set up write buffer for output file
        if(!(outputFormatContext->oformat->flags & AVFMT_NOFILE)){
            ret = avio_open(&outputFormatContext->pb, outputFileName.c_str(),AVIO_FLAG_WRITE);
            if(ret < 0){
                std::cout << "Could not open output file: " << outputFileName;
                cleanUp(streamsList, ret);
            }
        }
    
    AVDictionary* opts = NULL;
    // write header for output file
    // we pass the _adress_ of the AVDictionary pointer
    ret = avformat_write_header(outputFormatContext, &opts);
    if (ret < 0) {
        std::cout << "Error occured when opening output file! \n";
        cleanUp(streamsList, ret);
    }
    
    // here we start to copy the packets
    while(true) {
        // variables to point at the streams.
        AVStream *inStream, *outStream;
        // read a packet from the stream. It will be assigned to the variable packet
        ret = av_read_frame(inputFormatContext, &packet);
        // av_read_frame() returns 0 if packet read was successful, otherwise a negative int
        if (ret < 0) {
            //something went wrong, exit
            break;
        }
        // get the stream from our input file
        inStream = inputFormatContext->streams[packet.stream_index];
        if(packet.stream_index >= numStreams || streamsList[packet.stream_index] < 0) {
            // packet should not be muxed, discard
            av_packet_unref(&packet);
            continue;
        }
        
        packet.stream_index = streamsList[packet.stream_index]; // assign new index for the output
        outStream = outputFormatContext->streams[packet.stream_index]; // point outStream correctly
        // copy the packet
        // we cast the last argument to AVRounding because C++ is stricter than C with enums
        packet.pts = av_rescale_q_rnd(packet.pts, inStream->time_base, outStream->time_base, (AVRounding)(AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        packet.dts = av_rescale_q_rnd(packet.dts, inStream->time_base, outStream->time_base, (AVRounding) (AV_ROUND_NEAR_INF|AV_ROUND_PASS_MINMAX));
        packet.duration = av_rescale_q(packet.duration,inStream->time_base,outStream->time_base);
        // since we don't know what position packet will have in the final stream, set to -1 for unknown
        packet.pos = -1;
        
        // write the packet to file
        //https://ffmpeg.org/doxygen/trunk/group__lavf__encoding.html#ga37352ed2c63493c38219d935e71db6c1
        ret = av_interleaved_write_frame(outputFormatContext, &packet);
        if(ret < 0) {
            std::cout << "Error muxing packet!";
            break;
        }
        // ALWAYS unref the packet when you're done!
        av_packet_unref(&packet);
        
    }
    
    av_write_trailer(outputFormatContext);
    return cleanUp(streamsList, ret);
}

int Transmuxer::cleanUp(int* streamList, int &ret) {
    /**
                Clean the contexts used  when transmuxing
     */
    // close input context
    avformat_close_input(&inputFormatContext);
    // TODO: complete method
    if(!outputFormatContext && !(outputFormatContext->oformat->flags & AVFMT_NOFILE)) {
        avio_closep(&outputFormatContext->pb);
    }
    avformat_free_context(outputFormatContext);
    av_freep(&streamList);
    if(ret < 0 && ret != AVERROR_EOF){
        std::cout << "An error occured: \n";
        std::cout << "\t" << av_err2str(ret);
        return 1;
    }
    return 0;
};

