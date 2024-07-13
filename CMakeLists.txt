# CMakeLists.txt

cmake_minimum_required(VERSION 3.22) # set minimum required CMake version

project(GameOfLife) # set project name

set(CMAKE_CXX_STANDARD 17) # Set C^++ standard version

# Add compiler flags:
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -O3 -fno-tree-vectorize -fopenmp -I~/Vc/include -L~/Vc/lib -I/usr/include/gegl-0.4 -L/usr/lib64 /usr/lib64/libOpenCL.so.1")

# Add compiler flags for debug mode:
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_RELEASE} -g -Wall")

# Include directories
include_directories(${PROJECT_SOURCE_DIR}/include)

# Source files
set(SOURCES
    src/GameOfLife.cpp
    src/World.cpp
    src/OpenCLWrapper.cpp
    src/cli.cpp
)

# Create executable
add_executable(GameOfLife ${SOURCES})

# Link against Vc library
target_link_libraries(GameOfLife $ENV{HOME}/Vc/lib/libVc.a)



# Command to build using CMake
#mkdir build # create build directory
#cd build # move into build directory
#cmake -DCMAKE_BUILD_TYPE=Release ^.. # meta-build with cmake file in previous directory
#make # build
