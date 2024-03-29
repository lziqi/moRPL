#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

#include "prpl-dataManager.h"
#include "prpl-transition.h"

#include <CL/cl.h>
#include <spdlog/spdlog.h>
#include "spdlog/cfg/env.h"
#include "morpl-Error.h"
#include "demoTrans.h"
#include <time.h>

using namespace std;

int main(int argc, char *argv[])
{
    spdlog::cfg::load_env_levels();
    // 初始化DataManager
    bool withWriter = 0;
    pRPL::DataManager dataManager;
    if (!dataManager.initMPI(MPI_COMM_WORLD, withWriter, pRPL::DeviceOption::GPU_ONLY))
    {
        cerr << "Error: unable to initialize MPI" << endl;
        return -1;
    }

    double start_time, end_time;
    dataManager.mpiPrc().sync();
    if (dataManager.mpiPrc().isMaster())
        start_time = MPI_Wtime();

    /*初始化参数*/
    int nRowSubspcs = 1;
    int nColSubspcs = 1;

    pRPL::ReadingOption readOpt = pRPL::CENTDEL_READING;
    pRPL::WritingOption writeOpt = pRPL::CENTDEL_WRITING; // pRPL::CENTDEL_WRITING

    /*加入输入图层*/
    pRPL::Layer *inLayer = dataManager.addLayerByGDAL("inputLayer", "../data/resample/dg2001_1.tif", 1, true); //./data/resample/dg2001_1.tif
    pRPL::Layer *limitLayer = dataManager.addLayerByGDAL("limitLayer", "../data/resample/river2001_1.tif", 1, true);
    pRPL::Layer *probLayer = dataManager.addLayerByGDAL("probLayer", "../data/resample/prob2001_1.tif", 1, true);

    const pRPL::SpaceDims &spaceDim = *(inLayer->glbDims());
    const pRPL::CellspaceGeoinfo *cellspaceGeoInfo = inLayer->glbGeoinfo();
    long tileSize = inLayer->tileSize();

    /*初始化输出图层*/
    pRPL::Layer *outLayer = dataManager.addLayer("outputLayer");
    outLayer->initCellspaceInfo(spaceDim, typeid(unsigned char).name(), sizeof(unsigned char), cellspaceGeoInfo, tileSize);

    /*添加邻域图层*/
    pRPL::Neighborhood *neighborhood3x3 = dataManager.addNbrhd("Moore3x3");
    neighborhood3x3->initMoore(3, 1.0, pRPL::FORBID_VIRTUAL_EDGES, 0);

    /*初始化transition*/
    // pRPL::Transition transition;
    DemoTransition transition;
    transition.setNbrhdByName(neighborhood3x3->name());
    // transition.addInputLyr(inLayer->name(), false);
    // transition.addInputLyr(limitLayer->name(), false);
    // transition.addInputLyr(probLayer->name(), false);

    /*创建输出图层*/
    string outLayerName;
    outLayerName.assign("./output/OpenCL_" + to_string(dataManager.mpiPrc().nProcesses()) + ".tif");
    for (int i = 0; i < 1; i++)
    {
        if (i % 2 == 0)
        {
            transition.addInputLyr(inLayer->name(), false);
            transition.addInputLyr(limitLayer->name(), false);
            transition.addInputLyr(probLayer->name(), false);
            transition.addOutputLyr(outLayer->name(), true);

            /*分解图层*/
            spdlog::debug("分解图层");
            if (!dataManager.dcmpLayers(transition, nRowSubspcs, nColSubspcs))
            {
                dataManager.mpiPrc().abort();
                return -1;
            }

            dataManager.mpiPrc().sync();

            if (!dataManager.createLayerGDAL(outLayer->name(), outLayerName.c_str(), "GTiff", NULL))
            {
                dataManager.mpiPrc().abort();
                return -1;
            }
        }
        else
        {
            transition.addInputLyr(outLayer->name(), false);
            transition.addInputLyr(limitLayer->name(), false);
            transition.addInputLyr(probLayer->name(), false);
            transition.addOutputLyr(inLayer->name(), true);

            /*分解图层*/
            spdlog::debug("分解图层");
            if (!dataManager.dcmpLayers(transition, nRowSubspcs, nColSubspcs))
            {
                dataManager.mpiPrc().abort();
                return -1;
            }

            dataManager.mpiPrc().sync();
            if (!dataManager.createLayerGDAL(inLayer->name(), outLayerName.c_str(), "GTiff", NULL))
            {
                dataManager.mpiPrc().abort();
                return -1;
            }
        }
        dataManager.mpiPrc().sync();

        pRPL::pCuf pf = &pRPL::Transition::ocLocalSegmentOperator;//ocLocalSegmentOperator; ocLocalOperator

        dataManager.allocSubspc();
        /*开始计算任务*/
        spdlog::debug("初始化任务");
        if (!dataManager.initStaticTask(transition, pRPL::CYLC_MAP, readOpt))
        {
            return -1;
        }

        // int subspcs2Map = 2 * (dataManager.mpiPrc().nProcesses() - 1);
        // if (!dataManager.initTaskFarm(transition, pRPL::CYLC_MAP, subspcs2Map, readOpt))
        // {
        //     return -1;
        // }

        dataManager.mpiPrc().sync();

        /* GPU设备信息: */
        cl_device_id gpuID = dataManager.getProcess().getNode().getGPUID();
        spdlog::debug("main中获取到的GPU设备信息:");
        // cout << gpuID << endl;

        if (!transition.initOpenCL("OpenCL/add.cl", "rf_ca", gpuID))
        {
            spdlog::error("Init OpenCL Error");
            return -1;
        }

        double execStartTime, execEndTime;
        dataManager.mpiPrc().sync();
        if (dataManager.mpiPrc().isMaster())
            execStartTime = MPI_Wtime();
        /* 使用GPU计算 */
        if (dataManager.evaluate_ST(pRPL::EVAL_ALL, transition, writeOpt, true, pf, false) != pRPL::EVAL_SUCCEEDED)
        {
            return -1;
        }
        // if (dataManager.evaluate_TF(pRPL::EVAL_ALL, transition, readOpt, writeOpt, false, pf) != pRPL::EVAL_SUCCEEDED)
        // {
        //     return -1;
        // }

        dataManager.mpiPrc().sync();
        if (dataManager.mpiPrc().isMaster())
            execEndTime = MPI_Wtime();

        spdlog::debug("GPU计算耗时:{} ms", (execEndTime - execStartTime) * 1000);

        transition.clearLyrSettings();
    }

    /*结束*/
    dataManager.closeDatasets();

    // Record the end time, log computing time
    dataManager.mpiPrc().sync();
    if (dataManager.mpiPrc().isMaster())
    {
        end_time = MPI_Wtime();
        spdlog::info("程序总耗时: {} ms.", (end_time - start_time) * 1000);
    }

    // Finalize MPI
    dataManager.finalizeMPI();

    return 0;
}
