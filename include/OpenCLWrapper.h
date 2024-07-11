/*
* An OpenCLWrapper for only one specific use-case: evolve function of the World class.
* Could be expanded.
*/

#ifndef OPENCLWRAPPER_H
#define OPENCLWRAPPER_H

class World;

#include <gegl-0.4/opencl/cl.h>

#include <string>

class OpenCLWrapper {
public:
    cl_int err;
    cl_uint platformCount;
    cl_platform_id platform;
    cl_uint deviceCount;
    cl_command_queue queue;
    cl_kernel kernel;
    cl_mem buffer_grid;
    cl_mem buffer_newGrid;
    size_t global_work_size[2];

    cl_program program;
    cl_context context;
    cl_device_id device;

    /**
     * @brief Default constructor. Only a placeholder for now.
     */
    OpenCLWrapper() { };

    /**
     * @brief Construct a new OpenCL object.
     * Initializes everything needed for OpenCL computation in order to allow recurrent use without overhead.
     *
     */
    OpenCLWrapper(World& world);

    ~OpenCLWrapper();

    std::string readKernelSource(const char* filename);

    void checkError(cl_int err, const char* operation);

    void printAttributes();

private:
    
};

#endif // WORLD_H
