#include <iostream>
#include "AV/src/transmuxer.hpp"
#include "AV/src/transcoder.hpp"

int main(int argc, char* argv[]) {
    if(argc < 6) {
        std::cout << "you have not provided enough arguments!";
        return -1;
    }
    Transcoder transcoder = Transcoder();
    std::string input = std::string(argv[1]);
    std::string output = std::string(argv[2]);
    std::string codec = std::string(argv[3]);
    std::string codecPrivKey = std::string(argv[4]);
    std::string codecPrivValue = std::string(argv[5]);
    int response = transcoder.Transcode(input, output, codec,codecPrivKey, codecPrivValue);
    //std::cout << "Builds and runs! \n";
    return response; 
}
