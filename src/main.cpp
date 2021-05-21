#include <iostream>
#include "AV/transmuxer.hpp"

int main(int argc, char* argv[]) {
    if(argc < 3) {
        std::cout << "you have not provided enough arguments!";
        return -1;
    }
    Transmuxer transmuxer = Transmuxer();
    std::string input = std::string(argv[1]);
    std::string output = std::string(argv[2]);
    int response = transmuxer.transmux(input, output);
    std::cout << "Builds and runs! \n";
    return response; 
}
