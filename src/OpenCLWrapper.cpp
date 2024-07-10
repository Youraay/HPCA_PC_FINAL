#include "OpenCLWrapper.h"
#include "World.h"

#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

OpenCLWrapper::OpenCLWrapper(World& world) {
  cl_uint N = world.width * world.height; 
  int* newGrid = new int[N];

  cl_int err;

  cl_uint platformCount;
  cl_platform_id platform;
  err = clGetPlatformIDs(1, &platform, &platformCount);
  checkError(err, "clGetPlatformIDs");

  cl_uint deviceCount;
  cl_device_id device;
  err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_ALL, 1, &device, &deviceCount);
  checkError(err, "clGetDeviceIDs");

  cl_context context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
  checkError(err, "clCreateContext");

  cl_command_queue queue = clCreateCommandQueue(context, device, 0, &err);
  checkError(err, "clCreateCommandQueue");

  cl_mem buffer_grid = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * N, world.grid, &err);
  checkError(err, "clCreateBuffer (buffer_grid)");
  cl_mem buffer_newGrid = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(int) * N, NULL, &err);
  checkError(err, "clCreateBuffer (buffer_newGrid)");

  std::string sourceStr = readKernelSource("evolve.cl");
  const char* source = sourceStr.c_str();
  cl_program program = clCreateProgramWithSource(context, 1, &source, NULL, &err);
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

  cl_kernel kernel = clCreateKernel(program, "evolve", &err);
  checkError(err, "clCreateKernel");

  err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &buffer_grid);
  checkError(err, "clSetKernelArg (bufferA)");
  err = clSetKernelArg(kernel, 1, sizeof(cl_mem), &buffer_newGrid);
  checkError(err, "clSetKernelArg (bufferB)");
  err = clSetKernelArg(kernel, 2, sizeof(int), &world.width);
  checkError(err, "clSetKernelArg (bufferC)");
  err = clSetKernelArg(kernel, 3, sizeof(int), &world.height);
  checkError(err, "clSetKernelArg (N)");

  size_t global_work_size[2] = { (size_t)world.width, (size_t)world.height };
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


