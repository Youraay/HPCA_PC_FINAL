#include "OpenCLWrapper.h"
#include "World.h"

#include <vector>
#include <iostream>
#include <filesystem>
#include <fstream>

OpenCLWrapper::OpenCLWrapper(World& world) {
    // All this debugging text is needed because we can't install additional debugging info without sudo rights.
    std::cout << "OpenCL: Getting platform IDs..." << std::endl;
    err = clGetPlatformIDs(1, &platform, &platformCount);
    checkError(err, "clGetPlatformIDs");

    std::cout << "OpenCL: Getting device IDs..." << std::endl;
    err = clGetDeviceIDs(platform, CL_DEVICE_TYPE_GPU, 1, &device, &deviceCount);
    checkError(err, "clGetDeviceIDs");

    std::cout << "OpenCL: Creating context..." << std::endl;
    context = clCreateContext(NULL, 1, &device, NULL, NULL, &err);
    checkError(err, "clCreateContext");

    std::cout << "OpenCL: Creating command queue..." << std::endl;
    queue = clCreateCommandQueue(context, device, 0, &err);
    // COMMENT ABOVE AND UNCOMMENT BELOW FOR PROFILING.
    //queue = clCreateCommandQueue(context, device, CL_QUEUE_PROFILING_ENABLE, &err);
    checkError(err, "clCreateCommandQueue");

    std::cout << "OpenCL: Reading kernel source..." << std::endl;
    std::string sourceStr = readKernelSource("../src/evolve_and_compare.cl");
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

    std::cout << "OpenCL: Creating kernels..." << std::endl;
    kernel_evolve = clCreateKernel(program, "evolve", &err);
    checkError(err, "clCreateKernel (evolve)");
    kernel_compare = clCreateKernel(program, "compare_arrays", &err);
    checkError(err, "clCreateKernel (compare)");

    std::cout << "OpenCL: Creating buffers..." << std::endl;
    // Buffer for the current grid (evolve).
    buffer_grid = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(bool) * world.N, NULL, &err);
    checkError(err, "clCreateBuffer (buffer_grid)");
    // Buffer for the new grid (evolve).
    buffer_newGrid = clCreateBuffer(context, CL_MEM_WRITE_ONLY, sizeof(bool) * world.N, NULL, &err);
    checkError(err, "clCreateBuffer (buffer_newGrid)");
    // Buffer for the first grid (compare)
    buffer_grid1 = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(bool) * world.N, NULL, &err);
    checkError(err, "clCreateBuffer (buffer_grid1)");
    // Buffer for the second grid (compare)
    buffer_grid2 = clCreateBuffer(context, CL_MEM_READ_ONLY, sizeof(bool) * world.N, NULL, &err);
    checkError(err, "clCreateBuffer (buffer_grid2)");
    // Buffer for the comparison result (compare)
    buffer_result = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(int), NULL, &err);
    checkError(err, "clCreateBuffer (buffer_result)");

    std::cout << "OpenCL: Setting kernel arguments..." << std::endl;
    // Set the arguments for the evolve kernel.
    err = clSetKernelArg(kernel_evolve, 0, sizeof(cl_mem), &buffer_grid);
    checkError(err, "clSetKernelArg (buffer_grid)");
    err = clSetKernelArg(kernel_evolve, 1, sizeof(cl_mem), &buffer_newGrid);
    checkError(err, "clSetKernelArg (buffer_newGrid)");
    err = clSetKernelArg(kernel_evolve, 2, sizeof(int), &world.width);
    checkError(err, "clSetKernelArg (width)");
    err = clSetKernelArg(kernel_evolve, 3, sizeof(int), &world.height);
    checkError(err, "clSetKernelArg (height)");
    // Set the arguments for the compare kernel.
    err = clSetKernelArg(kernel_compare, 0, sizeof(cl_mem), &buffer_grid1);
    checkError(err, "clSetKernelArg (buffer_grid1)");
    err = clSetKernelArg(kernel_compare, 1, sizeof(cl_mem), &buffer_grid2);
    checkError(err, "clSetKernelArg (buffer_grid2)");
    err = clSetKernelArg(kernel_compare, 2, sizeof(cl_mem), &buffer_result);
    checkError(err, "clSetKernelArg (result)");

    err = clSetKernelArg(kernel_compare, 3, sizeof(ulong), &world.N);
    checkError(err, "clSetKernelArg (N)");

    evolve_global_work_size[0] = (size_t)world.width;
    evolve_global_work_size[1] = (size_t)world.height;
    //evolve_local_work_size[0] = 16;
    //evolve_local_work_size[1] = 16;

    compare_global_work_size[0] = world.N;
    //compare_local_work_size[0] = 16;

    std::cout << "OpenCL Initialized!" << std::endl;
    printAttributes(platform, device);
}

OpenCLWrapper::~OpenCLWrapper() {
    clReleaseMemObject(buffer_newGrid);
    clReleaseMemObject(buffer_grid);
    clReleaseMemObject(buffer_grid1);
    clReleaseMemObject(buffer_grid2);
    clReleaseMemObject(buffer_result);
    clReleaseKernel(kernel_evolve);
    clReleaseKernel(kernel_compare);
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

void OpenCLWrapper::printAttributes(cl_platform_id platform, cl_device_id device) {
    char info[1024];

    // Platform Info
    clGetPlatformInfo(platform, CL_PLATFORM_NAME, sizeof(info), info, NULL);
    std::cout << "Platform: " << info << std::endl;
    clGetPlatformInfo(platform, CL_PLATFORM_VENDOR, sizeof(info), info, NULL);
    std::cout << "Vendor: " << info << std::endl;
    clGetPlatformInfo(platform, CL_PLATFORM_VERSION, sizeof(info), info, NULL);
    std::cout << "Version: " << info << std::endl;
    clGetPlatformInfo(platform, CL_PLATFORM_PROFILE, sizeof(info), info, NULL);
    std::cout << "Profile: " << info << std::endl;

    // Device Info
    clGetDeviceInfo(device, CL_DEVICE_NAME, sizeof(info), info, NULL);
    std::cout << "Device: " << info << std::endl;
    clGetDeviceInfo(device, CL_DEVICE_VENDOR, sizeof(info), info, NULL);
    std::cout << "Vendor: " << info << std::endl;
    clGetDeviceInfo(device, CL_DEVICE_VERSION, sizeof(info), info, NULL);
    std::cout << "Version: " << info << std::endl;
    clGetDeviceInfo(device, CL_DRIVER_VERSION, sizeof(info), info, NULL);
    std::cout << "Driver Version: " << info << std::endl;

    cl_uint compute_units;
    clGetDeviceInfo(device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(compute_units), &compute_units, NULL);
    std::cout << "Compute Units: " << compute_units << std::endl;

    size_t work_group_size;
    clGetDeviceInfo(device, CL_DEVICE_MAX_WORK_GROUP_SIZE, sizeof(work_group_size), &work_group_size, NULL);
    std::cout << "Max Work Group Size: " << work_group_size << std::endl;

    cl_ulong global_mem_size;
    clGetDeviceInfo(device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(global_mem_size), &global_mem_size, NULL);
    std::cout << "Global Memory Size: " << global_mem_size / (1024 * 1024) << " MB" << std::endl;

    cl_ulong local_mem_size;
    clGetDeviceInfo(device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(local_mem_size), &local_mem_size, NULL);
    std::cout << "Local Memory Size: " << local_mem_size / 1024 << " KB" << std::endl;

    clGetDeviceInfo(device, CL_DEVICE_OPENCL_C_VERSION, sizeof(info), info, NULL);
    std::cout << "OpenCL C Version: " << info << std::endl << std::endl;

    std::cout << "OpenCLWrapper Attributes:" << std::endl;
    std::cout << "  queue: " << queue << std::endl;
    std::cout << "  kernel_evolve: " << kernel_evolve << std::endl;
    std::cout << "  kernel_compare: " << kernel_compare << std::endl;
    std::cout << "  buffer_newGrid: " << buffer_newGrid << std::endl;
    std::cout << "  evolve_global_work_size: [" << evolve_global_work_size[0] << ", " << evolve_global_work_size[1] << "]" << std::endl;
    std::cout << "  compare_global_work_size: [" << compare_global_work_size[0] << "]" << std::endl;
    std::cout << "  context: " << context << std::endl;
    std::cout << "  program: " << program << std::endl;
    std::cout << "  device: " << device << std::endl;
    std::cout << "  err: " << err << std::endl;
}

