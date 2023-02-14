
#ifndef GPU_H
#define GPU_H
#include <vector>
#include <string>
#include <iostream>
#include <CL/cl.h>
#include "morpl-Error.h"
#include "prpl-basicTypes.h"
using namespace std;

namespace moRPL
{
    class Node
    {
    private:
        /* 当前进程的 */
        int id;
        string name;        /* 节点名 */
        cl_device_id gpuID; /* 当前进程的GPUID */

        /* 当前节点的 */
        int gpuCount;                /* 设备上GPU数 */
        vector<cl_device_id> gpuIDs; /* 设备上GPU ID表 */
        vector<cl_ulong> gpuMemorys;/* 设备上GPU显存大小 */
    public:
        int getID();
        string getName();
        cl_device_id getGPUID();

        int getGPUCount();
        vector<cl_device_id> getGPUIDs();
        vector<cl_ulong> getGPUMemorys();

        void setID(int id);
        void setName(string name);
        void setGPUID(cl_device_id id);

        /* 没有GPU返回false */
        bool valid();

        /* 初始化节点 */
        bool init(pRPL::DeviceOption deviceOption);
    };
}

#endif