#ifndef OPENCL_MANAGER__H
#define OPENCL_MANAGER__H

#include <stdio.h>
#include <CL/cl.h>
#include "morpl-Error.h"

/* 管理OpenCL的创建 */
namespace moRPL
{
    class OpenCLManager
    {
    private:
        cl_device_id device_id;
        cl_context context;
        cl_command_queue commandQueue;
        cl_program program;
        cl_kernel kernel;

    private:
        char *readKernel(const char *filename);

    public: /* get */
        cl_device_id getDeviceID();

        cl_context getContext();
        cl_command_queue getCommandQueue();
        cl_program getProgram();
        cl_kernel getKernel();

    public:
        /* 清理OpenCL资源 */
        bool clear();

        /* 设置device_id */
        void setDeviceID(cl_device_id device_id);

        /*创建Context*/
        bool createContext();

        /*创建队列*/
        bool createCommandQueue();

        /*创建OpenCL程序*/
        bool createProgram(const char *filename);

        /* 创建内核执行 */
        bool createKernel(const char *kernelname);
    };
}

#endif