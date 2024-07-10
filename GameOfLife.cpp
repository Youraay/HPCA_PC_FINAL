#include "World.cpp"
#include "cli.cpp"

#include <iostream>
#include <sstream>
#include <string>
#include <vector>


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
