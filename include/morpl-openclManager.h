#ifndef OPENCL_MANAGER__H
#define OPENCL_MANAGER__H

#include <stdio.h>
#include <iostream>
#include <CL/cl.h>
#include "morpl-Error.h"

/* 管理OpenCL的创建 */
namespace moRPL
{
    class OpenCLManager
    {
    private:
        /* 设备信息 */
        cl_ulong globalSize;

        cl_device_id device_id;
        cl_context context;
        cl_command_queue commandQueue;
        cl_program program;
        cl_kernel kernel;
    private:
        char *readKernel(const char *filename);

    public: /* get */
        cl_ulong getGlobalSize();

        cl_device_id getDeviceID();

        cl_context getContext();
        cl_command_queue getCommandQueue();
        cl_program getProgram();
        cl_kernel getKernel();

    public:
        /*
            获取设备信息部分
         */

        /* 获取设备内存大小(对GPU则为显存) */
        bool readGlobalSize();

        /*
            创建执行部分
         */

        /* 清理OpenCL资源 */
        bool close();

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