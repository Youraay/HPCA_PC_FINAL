#include "OpenCLWrapper.h"
#include "World.h"

#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

OpenCLWrapper::OpenCLWrapper(World& world) {
    cl_uint N = world.width * world.height; 
    int* newGrid = new int[N];

    err = clGetPlatformIDs(1, &platform, &platformCount);
    checkError(err, "clGetPlatformIDs");

    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &deviceCount);
    checkError(err, "clGetDeviceIDs");

    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    checkError(err, "clCreateContext");

    queue = clCreateCommandQueue(context, device, 0, &err);
    checkError(err, "clCreateCommandQueue");

    buffer_newGrid = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * N, NULL, &err);
    checkError(err, "clCreateBuffer (buffer_newGrid)");

    std::string sourceStr = readKernelSource("../src/evolve.cl");
    const char* source = sourceStr.c_str();
    program = clCreateProgramWithSource(context, 1, &source, NULL, &err);
    checkError(err, "clCreateProgramWithSource");

    err = clBuildProgram(program, 1, &device, NULL, NULL, NULL);
    if (err != CL_SUCCESS) {
        size_t log_size;
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, 0, NULL, &log_size);
        std::vector<char> log(log_size);
        clGetProgramBuildInfo(program, device, CL_PROGRAM_BUILD_LOG, log_size, log.data(), NULL);
        std::cerr << "Error during operation 'clBuildProgram': " << err << std::endl;
        std::cerr << "Build log:" << std::endl << log.data() << std::endl;
        exit(1);
    }

    kernel = clCreateKernel(program, "evolve", &err);
    checkError(err, "clCreateKernel");

    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_grid);
    checkError(err, "clSetKernelArg (buffer_grid)");
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_newGrid);
    checkError(err, "clSetKernelArg (buffer_newGrid)");
    err = clSetKernelArg(kernel, 2, sizeof(int), &world.width);
    checkError(err, "clSetKernelArg (width)");
    err = clSetKernelArg(kernel, 3, sizeof(int), &world.height);
    checkError(err, "clSetKernelArg (height)");

    global_work_size[0] = (size_t)world.width;
    global_work_size[1] = (size_t)world.height;
}

OpenCLWrapper::~OpenCLWrapper() {
    clReleaseMemObject(buffer_grid);
    clReleaseMemObject(buffer_newGrid);
    clReleaseKernel(kernel);
    clReleaseProgram(program);
    clReleaseCommandQueue(queue);
    clReleaseContext(context);
}

std::string OpenCLWrapper::readKernelSource(const char* filename) {
    std::ifstream file(filename);
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}


void OpenCLWrapper::checkError(cl_int err, const char* operation) {
    if (err != CL_SUCCESS) {
        std::cerr << "Error during operation '" << operation << "': " << err << std::endl;
        exit(1);
    }
}

void OpenCLWrapper::printAttributes() {
    std::cout << "OpenCLWrapper Attributes:" << std::endl;
    std::cout << "  queue: " << queue << std::endl;
    std::cout << "  kernel: " << kernel << std::endl;
    std::cout << "  buffer_grid: " << buffer_grid << std::endl;
    std::cout << "  buffer_newGrid: " << buffer_newGrid << std::endl;
    std::cout << "  global_work_size: [" << global_work_size[0] << ", " << global_work_size[1] << "]" << std::endl;
    std::cout << "  context: " << context << std::endl;
    std::cout << "  program: " << program << std::endl;
    std::cout << "  device: " << device << std::endl;
    std::cout << "  err: " << err << std::endl;
}

