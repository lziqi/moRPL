#include "morpl-openclManager.h"

bool moRPL::OpenCLManager::close()
{
    if (commandQueue == 0 || !moRPL::checkCLError(clReleaseCommandQueue(commandQueue), __FILE__, __LINE__))
        return false;
    if (kernel == 0 || !moRPL::checkCLError(clReleaseKernel(kernel), __FILE__, __LINE__))
        return false;
    if (program == 0 || !moRPL::checkCLError(clReleaseProgram(program), __FILE__, __LINE__))
        return false;
    if (context == 0 || !moRPL::checkCLError(clReleaseContext(context), __FILE__, __LINE__))
        return false;
    return true;
}

/* Get */
cl_device_id moRPL::OpenCLManager::getDeviceID()
{
    return device_id;
}

cl_context moRPL::OpenCLManager::getContext()
{
    return context;
}

cl_command_queue moRPL::OpenCLManager::getCommandQueue()
{
    return commandQueue;
}

cl_program moRPL::OpenCLManager::getProgram()
{
    return program;
}

cl_kernel moRPL::OpenCLManager::getKernel()
{
    return kernel;
}

/* 设置device_id */
void moRPL::OpenCLManager::setDeviceID(cl_device_id device_id)
{
    this->device_id = device_id;
}

/* 创建context */
bool moRPL::OpenCLManager::createContext()
{
    cl_int err;
    context = clCreateContext(NULL, 1, &device_id, NULL, NULL, &err);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
    {
        spdlog::error("OpenCL: failed to create context for device");
        close();
        return false;
    }

    return true;
}

/* 创建执行队列 */
bool moRPL::OpenCLManager::createCommandQueue()
{
    cl_int err;
    /* CL_QUEUE_PROFILING_ENABLE启动分析，不需要可以填写 */
    commandQueue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, NULL);
    if (commandQueue == NULL)
    {
        spdlog::error("OpenCL: failed to create commandqueue for device");
        close();
        return false;
    }
    return true;
}

/* 读取内核文件 */
char *moRPL::OpenCLManager::readKernel(const char *filename)
{
    FILE *file = NULL;
    size_t sourceLength;
    char *sourceString;
    int ret = 0;

    file = fopen(filename, "rb");
    if (file == NULL)
    {
        spdlog::error("OpenCL: {} at {} , can't open {}", __FILE__, __LINE__, filename);
        return NULL;
    }
    fseek(file, 0, SEEK_END);
    sourceLength = ftell(file);
    fseek(file, 0, SEEK_SET);
    sourceString = (char *)malloc(sourceLength + 1);
    sourceString[0] = '\0';

    ret = fread(sourceString, sourceLength, 1, file);
    if (ret == 0)
    {
        spdlog::error("OpenCL: {} at {} , can't read source {}", __FILE__, __LINE__, filename);
        return NULL;
    }
    fclose(file);

    sourceString[sourceLength] = '\0';
    return sourceString;
}

/* 创建程序 */
bool moRPL::OpenCLManager::createProgram(const char *filename)
{
    cl_int err;

    char *const source = readKernel(filename);
    program = clCreateProgramWithSource(context, 1, (const char **)&source, NULL, NULL);

    if (program == NULL)
    {
        spdlog::error("OpenCL: failed to create CL program from source");
        return false;
    }

    err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
    {
        char buildLog[16384];
        clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);
        spdlog::error("OpenCL: error in kernel: {}", buildLog);
        clReleaseProgram(program);
        return false;
    }
    return true;
}

/* 创建内核执行 */
bool moRPL::OpenCLManager::createKernel(const char *kernelname)
{
    kernel = clCreateKernel(program, kernelname, NULL);
    if (kernel == NULL)
    {
        spdlog::error("OpenCL: failed to create kernel");
        close();
        return false;
    }
    return true;
}

/* 设备信息 */
bool moRPL::OpenCLManager::readGlobalSize()
{
    size_t size;
    cl_int err;

    clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, 0, NULL, &size);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        return false;
    cl_ulong globalSize;

    err = clGetDeviceInfo(device_id, CL_DEVICE_GLOBAL_MEM_SIZE, size, &globalSize, NULL);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        return false;

    this->globalSize = globalSize;
    return true;
}

cl_ulong moRPL::OpenCLManager::getGlobalSize()
{
    return this->globalSize;
}
