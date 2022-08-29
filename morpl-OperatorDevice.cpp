#include "morpl-OperatorDevice.h"

namespace moRPL
{
    /*读取内核文件*/
    char *ReadKernel(const char *filename)
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

    /*创建context*/
    cl_context CreateContext(cl_device_id *device_id)
    {
        cl_int err;
        cl_uint numPlatforms;
        cl_platform_id firstPlatformId;
        cl_context context = NULL;

        /* 获取第一个平台id */
        err = clGetPlatformIDs(1, &firstPlatformId, &numPlatforms);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
            return NULL;

        /* 获取GPU设备 */
        err = clGetDeviceIDs(firstPlatformId, CL_DEVICE_TYPE_GPU, 1, device_id, NULL);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        {
            err = clGetDeviceIDs(firstPlatformId, CL_DEVICE_TYPE_CPU, 1, device_id, NULL);
        }

        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        {
            return NULL;
        }

        context = clCreateContext(NULL, 1, device_id, NULL, NULL, &err);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        {
            return NULL;
        }

        return context;
    }

    /*使用context与设备id创建执行队列*/
    cl_command_queue CreateCommandQueue(cl_context context, cl_device_id device_id)
    {
        cl_int err;
        cl_command_queue commandQueue = NULL;

        /* CL_QUEUE_PROFILING_ENABLE启动分析，不需要可以填写 */
        commandQueue = clCreateCommandQueue(context, device_id, CL_QUEUE_PROFILING_ENABLE, NULL);
        if (commandQueue == NULL)
        {
            spdlog::error("OpenCL: failed to create commandqueue for device");
            return NULL;
        }
        return commandQueue;
    }

    /* 创建OpenCL程序 */
    cl_program CreateProgram(cl_context context, cl_device_id device_id, const char *filename)
    {
        cl_int err;
        cl_program program;

        char *const source = ReadKernel(filename);
        program = clCreateProgramWithSource(context, 1, (const char **)&source, NULL, NULL);

        if (program == NULL)
        {
            spdlog::error("OpenCL: failed to create CL program from source");
            return NULL;
        }

        err = clBuildProgram(program, 0, NULL, NULL, NULL, NULL);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        {
            char buildLog[16384];
            clGetProgramBuildInfo(program, device_id, CL_PROGRAM_BUILD_LOG, sizeof(buildLog), buildLog, NULL);
            spdlog::error("OpenCL: error in kernel: {}", buildLog);
            clReleaseProgram(program);
            return NULL;
        }
        return program;
    }

    /* 清理OpenCL资源 */
    bool CleanUp(cl_context context, cl_command_queue commandQueue, cl_program program, cl_kernel kernel, cl_event event)
    {
        if (commandQueue == 0 || !moRPL::checkCLError(clReleaseCommandQueue(commandQueue), __FILE__, __LINE__))
            return false;
        if (kernel == 0 || !moRPL::checkCLError(clReleaseKernel(kernel), __FILE__, __LINE__))
            return false;
        if (program == 0 || !moRPL::checkCLError(clReleaseProgram(program), __FILE__, __LINE__))
            return false;
        if (context == 0 || !moRPL::checkCLError(clReleaseContext(context), __FILE__, __LINE__))
            return false;
        if (event == 0 || !moRPL::checkCLError(clReleaseEvent(event), __FILE__, __LINE__))
            return false;
        return true;
    }

    cl_mem dataToGPU(cl_context context, pRPL::Cellspace *cellspace)
    {
        /* 当前图层的长宽 */
        long width = cellspace->info()->dims().nCols();
        long height = cellspace->info()->dims().nRows();

        /* 图层的GPU存储器对象 */
        cl_mem gpu_mem = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, width * height * cellspace->info()->dataSize(), cellspace->getData(), NULL);
        return gpu_mem;
    }
}