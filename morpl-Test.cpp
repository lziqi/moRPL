#include "morpl-Test.h"

namespace moRPL
{
    int testCL()
    {
        cl_context context = 0;
        cl_command_queue commandQueue = 0;
        cl_program program = 0;
        cl_device_id device_id = 0;
        cl_kernel kernel = 0;
        cl_mem memObjects[3] = {0, 0, 0};
        cl_int err;
        cl_event event;

        /* 创建上下文context */
        context = moRPL::CreateContext(&device_id);
        if (context == NULL)
        {
            printf("failed to create opencl context");
            return 1;
        }

        /* 创建队列 */
        commandQueue = moRPL::CreateCommandQueue(context, device_id);
        if (commandQueue == NULL)
        {
            moRPL::CleanUp(context, commandQueue, program, kernel, event);
            return 1;
        }

        /* 创建OpenCL程序 */
        program = moRPL::CreateProgram(context, device_id, "OpenCL/add.cl");
        if (program == NULL)
        {
            moRPL::CleanUp(context, commandQueue, program, kernel, event);
            return 1;
        }

        /* 创建OpenCL内核 */
        kernel = clCreateKernel(program, "test", NULL);
        if (kernel == NULL)
        {
            printf("failed to create kernel");
            moRPL::CleanUp(context, commandQueue, program, kernel, event);
            return 1;
        }

        /* 创建OpenCL内核对象 */
        float res[100][100];
        float a[100][100];
        float b[100][100];
        for (int i = 0; i < 100; i++)
        {
            for (int j = 0; j < 100; j++)
            {
                a[i][i] = (float)i * 100 + float(j);
                b[i][j] = (float)i * 100 + float(j);
            }
        }

        memObjects[0] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * 10000, a, NULL);
        memObjects[1] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float) * 10000, b, NULL);
        memObjects[2] = clCreateBuffer(context, CL_MEM_READ_WRITE, sizeof(float) * 10000, NULL, NULL);

        if (memObjects[0] == NULL || memObjects[1] == NULL || memObjects[2] == NULL)
        {
            printf("error creating memory objects");
            return 1;
        }

        /* 设置内核参数 */
        err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &memObjects[0]);
        err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &memObjects[1]);
        err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &memObjects[2]);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        {
            moRPL::CleanUp(context, commandQueue, program, kernel, event);
            return 1;
        }
        size_t globalWorkSize[2] = {100, 100};
        size_t localWorkSize[2] = {10, 10};

        /* 执行内核 */
        err = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, localWorkSize, 0, NULL, NULL);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        {
            moRPL::CleanUp(context, commandQueue, program, kernel, event);
            return 1;
        }

        /* 计算结果拷贝回本机 */
        err = clEnqueueReadBuffer(commandQueue, memObjects[2], CL_TRUE, 0, 10000 * sizeof(float), res, 0, NULL, NULL);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        {
            moRPL::CleanUp(context, commandQueue, program, kernel, event);
            return 1;
        }

        /* 打印结果 */
        for (int i = 0; i < 100; i++)
        {
            for (int j = 0; j < 100; j++)
            {
                printf("i=%d j=%d     %f\n", i, j, res[i][j]);
            }
        }
        printf("execute success!\n");
        return 0;
    }
}