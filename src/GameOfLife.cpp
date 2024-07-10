#include <iostream>
#include <ostream>
#include <stdexcept>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <string>
#include <vector>
#include <chrono>

#include "cli.h"



int main(int argc, char** argv) {
    //std::string filename = "testConfig.txt";
    //World test = World(filename);

    try {
        CommandLineInterface cli (argc, argv);
    } catch (const std::runtime_error& e) {
        std::cerr << e.what() << std::endl;
    }
    
    return 0;
}
