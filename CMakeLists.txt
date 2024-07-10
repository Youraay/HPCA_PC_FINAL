# CMakeLists.txt

cmake_minimum_required(VERSION 3.22) # set minimum required CMake version

project(GameOfLife) # set project name

set(CMAKE_CXX_STANDARD 17) # Set C^++ standard version

# Add compiler optimization level flag ”-O3”:
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3")

add_executable(GameOfLife # project name
    GameOfLife.cpp # main source file
)


# Command to build using CMake
#mkdir build # create build directory
#cd build # move into build directory
#cmake -DCMAKE_BUILD_TYPE=Release ^.. # meta-build with cmake file in previous directory
#make # build
