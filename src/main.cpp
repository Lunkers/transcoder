#include <iostream>
#include <yaml-cpp/yaml.h>
#include "AV/src/transmuxer.hpp"
#include "AV/src/transcoder.hpp"

int main(int argc, char* argv[]) {
//    if(argc < 3) {
//        std::cout << "you have not provided enough arguments!";
//        return -1;
//    }
    Transcoder transcoder = Transcoder();
    std::string input = std::string(argv[1]);
//    YAML::Node profile = YAML::LoadFile(argv[2]);
    StreamParams streamParams = {};
    streamParams.copyAudio = true;
    streamParams.copyVideo = false;
    streamParams.videoCodec = std::string("libx265");
    streamParams.codecPrivKey = std::string("x265-params");
    streamParams.codecPrivValue = std::string("keyint=60:min-keyint=60:scenecut=0");
    std::string output = "transcoded" + input;
    int response = transcoder.Transcode(input, output, streamParams);
    //std::cout << "Builds and runs! \n";
    return response; 
}
