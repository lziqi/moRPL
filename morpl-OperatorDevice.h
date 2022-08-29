#ifndef OPERATOR_DEVICE_H
#define OPERATOR_DEVICE_H

#include <CL/cl.h>
#include <stdio.h>

#include "prpl-cellspace.h"

#include "morpl-Error.h"

/*获取内核文件*/
/*将内核文件代码内容读取成char*字符串进行返回*/
namespace moRPL
{
    /*读取内核文件*/
    char *ReadKernel(const char *filename);

    /*创建Context*/
    cl_context CreateContext(cl_device_id *device_id);

    /*创建队列*/
    cl_command_queue CreateCommandQueue(cl_context context, cl_device_id device_id);

    /*创建OpenCL程序*/
    cl_program CreateProgram(cl_context context, cl_device_id device_id, const char *filename);

    /* 清理OpenCL资源 */
    bool CleanUp(cl_context context, cl_command_queue commandQueue, cl_program program, cl_kernel kernel, cl_event event);

    /* 1数据图层的数据转换为1存储器对象 */
    cl_mem dataToGPU(cl_context context, pRPL::Cellspace *cellspace);

    /*
    nbrSize:领域栅格栅格块总数
    nbrIndex:每个邻域栅格块的相对坐标位置
    width:tif的整个宽
    height:tif的整个高
    */
    /* 邻域栅格模块 */
    void focalOperator(int **inData, int **outData, int width, int height, int nbrSize, int *nbrIndex);
}

#endif