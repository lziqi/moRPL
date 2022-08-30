#include "morpl-node.h"

bool moRPL::Node::init()
{
    spdlog::info("节点设备初始化");
    cl_int err;

    cl_platform_id *platforms;
    cl_device_id *devices;

    cl_uint numPlatform;
    cl_uint numDevice;

    //获取平台数
    err = clGetPlatformIDs(0, NULL, &numPlatform);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        return false;
    // spdlog::info("平台数 : {}", numPlatform);

    //初始化平台
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * numPlatform);
    err = clGetPlatformIDs(numPlatform, platforms, NULL);

    //获得第一个平台上的GPU设备数量
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, 0, NULL, &numDevice);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        return false;
    devices = (cl_device_id *)malloc(sizeof(cl_device_id) * numDevice);
    this->gpuCount = numDevice;
    // spdlog::info("GPU设备数 : {}", numDevice);

    //获取GPU设备对应ID
    err = clGetDeviceIDs(platforms[0], CL_DEVICE_TYPE_GPU, numDevice, devices, NULL);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        return false;

    for (int i = 0; i < numDevice; i++)
        gpuIDs.push_back(devices[i]);

    /* Debug */
    spdlog::info("GPU的初始ID: ");
    for (int i = 0; i < numDevice; i++)
        cout << gpuIDs[i] << " ";
    cout << endl;

    return true;
    // devices = (cl_device_id *)malloc(sizeof(cl_device_id) * numDevice);
}

int moRPL::Node::getID()
{
    return this->id;
}

string moRPL::Node::getName()
{
    return this->name;
}

cl_device_id moRPL::Node::getGPUID()
{
    return this->gpuID;
}

int moRPL::Node::getGPUCount()
{
    return this->gpuCount;
}

vector<cl_device_id> moRPL::Node::getGPUIDs()
{
    return this->gpuIDs;
}

void moRPL::Node::setID(int id)
{
    this->id = id;
}

void moRPL::Node::setName(string name)
{
    this->name = name;
}

void moRPL::Node::setGPUID(cl_device_id id)
{
    this->gpuID = id;
}

bool moRPL::Node::valid()
{
    if (this->gpuIDs.empty())
    {
        return false;
    }
    return true;
}
