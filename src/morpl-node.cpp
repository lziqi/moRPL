#include "morpl-node.h"

bool moRPL::Node::init(pRPL::DeviceOption deviceOption)
{
    spdlog::info("-----OpenCL设备初始化-----");
    cl_int err;

    cl_platform_id *platforms;
    cl_device_id *devices;

    cl_uint numPlatform;
    cl_uint numDevice;
    int totalDevice = 0; //所有平台上的device总数

    //获取平台数
    err = clGetPlatformIDs(0, NULL, &numPlatform);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
        return false;
    spdlog::info("OpenCL平台数 : {}", numPlatform);

    //初始化平台
    platforms = (cl_platform_id *)malloc(sizeof(cl_platform_id) * numPlatform);
    err = clGetPlatformIDs(numPlatform, platforms, NULL);
    for (int i = 0; i < numPlatform; i++)
    {
        //获得平台上的GPU设备数量
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, 0, NULL, &numDevice);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
            return false;
        devices = (cl_device_id *)malloc(sizeof(cl_device_id) * numDevice);
        this->gpuCount = numDevice;
        spdlog::info("平台{}上GPU设备数 : {}", i, numDevice);

        //获取GPU设备对应ID
        err = clGetDeviceIDs(platforms[i], CL_DEVICE_TYPE_ALL, numDevice, devices, NULL);
        if (!moRPL::checkCLError(err, __FILE__, __LINE__))
            return false;

        /* 获取每个设备的设备信息 */
        vector<cl_device_type> device_types(numDevice);
        for (int j = 0; j < numDevice; j++)
        {
            size_t size;
            cl_device_type device_type;

            err = clGetDeviceInfo(devices[j], CL_DEVICE_TYPE, 0, NULL, &size);
            if (!moRPL::checkCLError(err, __FILE__, __LINE__))
                return false;

            err = clGetDeviceInfo(devices[j], CL_DEVICE_TYPE, size, &device_type, NULL);
            if (!moRPL::checkCLError(err, __FILE__, __LINE__))
                return false;
            device_types[j] = device_type;

            cl_ulong globalSize;
            clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, 0, NULL, &size);
            if (!moRPL::checkCLError(err, __FILE__, __LINE__))
                return false;

            err = clGetDeviceInfo(devices[j], CL_DEVICE_GLOBAL_MEM_SIZE, size, &globalSize, NULL);
            if (!moRPL::checkCLError(err, __FILE__, __LINE__))
                return false;
            spdlog::info("设备{}的内存 :{}", j, globalSize);
            // cout << globalSize << endl;
        }

        if (deviceOption == pRPL::DeviceOption::DEVICE_ALL)
        {
            for (int j = 0; j < numDevice; j++)
            {
                gpuIDs.push_back(devices[j]);
                totalDevice += 1;
            }
        }
        else
        {
            for (int j = 0; j < numDevice; j++)
            {
                if (deviceOption == device_types[j])
                {
                    gpuIDs.push_back(devices[j]);
                    totalDevice += 1;
                }
            }
        }
    }

    /* Debug */
    // spdlog::info("GPU的初始ID: ");
    // for (int i = 0; i < totalDevice; i++)
    //     cout << gpuIDs[i] << " ";
    // cout << endl;

    /* 销毁 */
    free(platforms);
    platforms = NULL;
    spdlog::info("-----OpenCL设备初始化完成-----");
    return true;
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
