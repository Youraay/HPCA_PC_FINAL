#include "OpenCLWrapper.h"
#include "World.h"

#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

OpenCLWrapper::OpenCLWrapper(World& world) {
    cl_uint N = world.width * world.height; 
    int* newGrid = new int[N];

    // All this debugging text is needed because we can't install additional debugging info without sudo rights.
    std::cout << "OpenCL: Getting platform IDs..." << std::endl;
    err = clGetPlatformIDs(1, &platform, &platformCount);
    checkError(err, "clGetPlatformIDs");

    std::cout << "OpenCL: Getting device IDs..." << std::endl;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &deviceCount);
    checkError(err, "clGetDeviceIDs");

    std::cout << "OpenCL: Creating context..." << std::endl;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    checkError(err, "clCreateContext");

    std::cout << "OpenCL: Creating command queue..." << std::endl;
    queue = clCreateCommandQueue(context, device, 0, &err);
    checkError(err, "clCreateCommandQueue");

    std::cout << "OpenCL: Creating buffers..." << std::endl;
    buffer_newGrid = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * N, NULL, &err);
    checkError(err, "clCreateBuffer (buffer_newGrid)");

    std::cout << "OpenCL: Reading kernel source..." << std::endl;
    std::string sourceStr = readKernelSource("../src/evolve.cl");
    const char* source = sourceStr.c_str();
    program = clCreateProgramWithSource(context, 1, &source, NULL, &err);
    checkError(err, "clCreateProgramWithSource");

    std::cout << "OpenCL: Building program..." << std::endl;
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

    std::cout << "OpenCL: Creating kernel..." << std::endl;
    kernel = clCreateKernel(program, "evolve", &err);
    checkError(err, "clCreateKernel");

    std::cout << "OpenCL: Setting kernel arguments..." << std::endl;
    err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_newGrid);
    checkError(err, "clSetKernelArg (buffer_newGrid)");
    err = clSetKernelArg(kernel, 2, sizeof(int), &world.width);
    checkError(err, "clSetKernelArg (width)");
    err = clSetKernelArg(kernel, 3, sizeof(int), &world.height);
    checkError(err, "clSetKernelArg (height)");

    global_work_size[0] = (size_t)world.width;
    global_work_size[1] = (size_t)world.height;

    std::cout << "OpenCL Initialized!" << std::endl;
    printAttributes();
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

