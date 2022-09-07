#include "prpl-transition.h"

pRPL::Transition::
    Transition(bool onlyUpdtCtrCell,
               bool needExchange,
               bool edgesFirst)
    : _pNbrhd(NULL),
      _onlyUpdtCtrCell(onlyUpdtCtrCell),
      _needExchange(needExchange),
      _edgesFirst(edgesFirst) {}

/*moRPL*/
bool pRPL::Transition::initOpenCL(const char *filename, const char *kernelname, cl_device_id device_id)
{
    openclManager.setDeviceID(device_id);

    if (!openclManager.createContext())
        return false;

    if (!openclManager.createCommandQueue())
        return false;

    if (!openclManager.createProgram(filename))
        return false;

    if (!openclManager.createKernel(kernelname))
        return false;

    return true;
}

pRPL::EvaluateReturn pRPL::Transition::ocLocalOperator(const pRPL::CoordBR &br)
{
    // cl_device_id device_id = this->deviceID;
    spdlog::info("-----邻域计算-----");

    cl_device_id device_id = openclManager.getDeviceID();
    cl_context context = openclManager.getContext();
    cl_command_queue commandQueue = openclManager.getCommandQueue();
    cl_program program = openclManager.getProgram();
    cl_kernel kernel = openclManager.getKernel();
    cl_event event;
    cl_int err;

    spdlog::info("ocLocal中GPU ID:");
    cout << device_id << endl;

    // cl_device_id device_id = 0;
    // cl_context context = 0;
    // cl_command_queue commandQueue = 0;
    // cl_program program = 0;
    // cl_kernel kernel = 0;
    // cl_event event;
    // cl_int err;

    // /* 创建上下文context */
    // context = moRPL::CreateContext(&device_id);
    // if (context == NULL)
    // {
    //     spdlog::error("OpenCL: failed to create opencl context");
    //     return pRPL::EvaluateReturn::EVAL_FAILED;
    // }

    // /* 创建队列 */
    // commandQueue = moRPL::CreateCommandQueue(context, device_id);
    // if (commandQueue == NULL)
    // {
    //     moRPL::CleanUp(context, commandQueue, program, kernel, event);
    //     return pRPL::EvaluateReturn::EVAL_FAILED;
    // }

    // string cl_path = "OpenCL/add.cl";

    // /* 创建OpenCL程序 */
    // program = moRPL::CreateProgram(context, device_id, cl_path.c_str());
    // if (program == NULL)
    // {
    //     moRPL::CleanUp(context, commandQueue, program, kernel, event);
    //     return pRPL::EvaluateReturn::EVAL_FAILED;
    // }

    // /* 创建OpenCL内核 */
    // kernel = clCreateKernel(program, "rf_ca", NULL);
    // if (kernel == NULL)
    // {
    //     spdlog::error("OpenCL: failed to create kernel");
    //     moRPL::CleanUp(context, commandQueue, program, kernel, event);
    //     return pRPL::EvaluateReturn::EVAL_FAILED;
    // }

    //左上角最小的x和最大的y，右下角最大的x与最小的y
    int minY = br.minIRow(); //西北方
    int maxY = br.maxIRow();
    int minX = br.minICol(); //西北方
    int maxX = br.maxICol();
    int brs[4] = {minX, minY, maxX, maxY};

    pRPL::Cellspace *cellspace = getCellspaceByLyrName(getPrimeLyrName());
    int height = cellspace->info()->dims().nRows();
    int width = cellspace->info()->dims().nCols();

    NbrInfo *nbrInfo = new NbrInfo;
    nbrInfo->nbrSize = _pNbrhd->size();

    vector<pRPL::WeightedCellCoord> coords = _pNbrhd->getNbrs();
    vector<int> nbrCoords;
    for (int i = 0; i < _pNbrhd->size(); i++)
    {
        nbrCoords.push_back(coords[i].iRow());
        nbrCoords.push_back(coords[i].iCol());
    }
    nbrInfo->nbrCoord = nbrCoords.data();

    /* 判断是否有邻域模块 */
    // if (minX != 0 || minY != 0 || maxX != (height - 1) || maxY != (width - 1))
    // {
    //     cerr << "you has defined a nrb,please use focal operator" << endl;
    //     return pRPL::EVAL_FAILED;
    // }

    double cellWidth = cellspace->info()->georeference()->cellSize().x();
    double cellHeight = cellspace->info()->georeference()->cellSize().y();

    //输入与输出的layer数量
    int numInLayers = _vInLyrNames.size();
    int numOutLayers = _vOutLyrNames.size();

    cl_mem *InData = new cl_mem[numInLayers];
    cl_mem *OutData = new cl_mem[numOutLayers];

    /* 输入图层数据 - 转到显存中 */
    for (int i = 0; i < numInLayers; i++)
    {
        cellspace = getCellspaceByLyrName(getInLyrNames()[i]);
        int height = cellspace->info()->dims().nRows();
        int width = cellspace->info()->dims().nCols();

        InData[i] = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, width * height * cellspace->info()->dataSize(), cellspace->getData(), NULL);
    }

    /* 输出图层数据 */
    for (int i = 0; i < numOutLayers; i++)
    {
        cellspace = getCellspaceByLyrName(getOutLyrNames()[i]);

        OutData[i] = clCreateBuffer(context, CL_MEM_READ_WRITE, width * height * cellspace->info()->dataSize(), NULL, NULL);
    }

    /* 邻域数据 */
    cl_mem clNbrCoords = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * nbrInfo->nbrSize * 2, (void *)nbrInfo->nbrCoord, NULL);
    cl_mem clNbrSize = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &(nbrInfo->nbrSize), NULL);

    /* tif长宽数据 */
    cl_mem clWidth = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &width, NULL);
    cl_mem clHeight = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int), &height, NULL);
    cl_mem clBR = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * 4, brs, NULL);

    /* 设置WorkSize与Step */
    cl_uint maxDimension;
    err = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS, sizeof(cl_uint), &maxDimension, NULL);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
    {
        spdlog::error("OpenCL: failed to get max work size");
        moRPL::CleanUp(context, commandQueue, program, kernel, event);
        return pRPL::EvaluateReturn::EVAL_FAILED;
    }

    size_t maxWorkSize[maxDimension];
    err = clGetDeviceInfo(device_id, CL_DEVICE_MAX_WORK_ITEM_SIZES, sizeof(size_t) * maxDimension, maxWorkSize, NULL);

    if (width < maxWorkSize[0])
        maxWorkSize[0] = width;
    if (height < maxWorkSize[1])
        maxWorkSize[1] = height;

    int step[2];
    step[0] = (width != maxWorkSize[0]) ? (width / maxWorkSize[0] + 1) : 1;
    step[1] = (height != maxWorkSize[1]) ? (height / maxWorkSize[1] + 1) : 1;
    cl_mem clStep = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(int) * 2, &step, NULL);

    /* thold */
    float thold = 0.06;
    cl_mem clThold = clCreateBuffer(context, CL_MEM_READ_ONLY | CL_MEM_COPY_HOST_PTR, sizeof(float), &thold, NULL);

    /* 设置内核参数 */
    /* 先用一个图层来测试 */
    err = clSetKernelArg(kernel, 0, sizeof(cl_mem), &InData[0]);
    err |= clSetKernelArg(kernel, 1, sizeof(cl_mem), &OutData[0]);
    err |= clSetKernelArg(kernel, 2, sizeof(cl_mem), &clWidth);
    err |= clSetKernelArg(kernel, 3, sizeof(cl_mem), &clHeight);
    err |= clSetKernelArg(kernel, 4, sizeof(cl_mem), &clBR);
    err |= clSetKernelArg(kernel, 5, sizeof(cl_mem), &clStep);
    err |= clSetKernelArg(kernel, 6, sizeof(cl_mem), &clNbrCoords);
    err |= clSetKernelArg(kernel, 7, sizeof(cl_mem), &clNbrSize);
    err |= clSetKernelArg(kernel, 8, sizeof(int) * nbrInfo->nbrSize * 2, NULL);
    err |= clSetKernelArg(kernel, 9, sizeof(cl_mem), &InData[1]);  // limit layer
    err |= clSetKernelArg(kernel, 10, sizeof(cl_mem), &InData[2]); // prob layer
    err |= clSetKernelArg(kernel, 11, sizeof(cl_mem), &clThold);   // thold

    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
    {
        spdlog::error("OpenCL: failed to set kernel arg");
        moRPL::CleanUp(context, commandQueue, program, kernel, event);
        return pRPL::EvaluateReturn::EVAL_FAILED;
    }

    spdlog::info("OpenCL参数信息 : maxWorkSize {} {} , step {} {}", maxWorkSize[0], maxWorkSize[1], step[0], step[1]);
    size_t globalWorkSize[2] = {maxWorkSize[0], maxWorkSize[1]};
    size_t localWorkSize[2] = {1, 1};

    err = clEnqueueNDRangeKernel(commandQueue, kernel, 2, NULL, globalWorkSize, NULL, 0, NULL, &event); //&event
    clWaitForEvents(1, &event);
    err = clFinish(commandQueue); //堵塞到命令队列中所有命令执行完毕

    cl_ulong startTime = 0, endTime = 0;
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_START,
                            sizeof(cl_ulong), &startTime, NULL);
    clGetEventProfilingInfo(event, CL_PROFILING_COMMAND_END,
                            sizeof(cl_ulong), &endTime, NULL);

    cl_ulong kernelExecTimeNs = endTime - startTime;

    /* 执行时间 */
    spdlog::info("Kernel耗时 : {} ms", kernelExecTimeNs * 1e-6);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
    {
        spdlog::error("OpenCL: failed to exec kernel");
        moRPL::CleanUp(context, commandQueue, program, kernel, event);
        return pRPL::EvaluateReturn::EVAL_FAILED;
    }

    /* 计算结果拷贝回内存 */
    void *res = malloc(width * height * cellspace->info()->dataSize());

    err = clEnqueueReadBuffer(commandQueue, OutData[0], CL_TRUE, 0, width * height * cellspace->info()->dataSize(), res, 0, NULL, NULL);
    if (!moRPL::checkCLError(err, __FILE__, __LINE__))
    {
        spdlog::error("OpenCL: failed to gpu to mem");
        moRPL::CleanUp(context, commandQueue, program, kernel, event);
        return pRPL::EvaluateReturn::EVAL_FAILED;
    }

    clock_t start, end;
    start = clock();

    for (int i = 0; i < numOutLayers; i++)
    {
        cellspace = getCellspaceByLyrName(getOutLyrNames()[i]);
        cellspace->updateData(res, cellspace->info()->dataSize() * width * height);
        cellspace->updateCellspaceAs(br, res);
    }

    end = clock();
    // spdlog::info("写回Cellspace耗时: {} ms", (end - start) / 1000);

    /* 清理内存 */
    free(res);
    delete nbrInfo;

    /* 清理cl_mem */
    if (!moRPL::checkCLError(clReleaseMemObject(InData[0]), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(InData[1]), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(InData[2]), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(clThold), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(OutData[0]), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(clWidth), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(clHeight), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(clBR), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(clStep), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(clNbrCoords), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    if (!moRPL::checkCLError(clReleaseMemObject(clNbrSize), __FILE__, __LINE__))
        return pRPL::EVAL_FAILED;
    /* 清理GPU端 */
    if (!moRPL::CleanUp(context, commandQueue, program, kernel, event))
        return pRPL::EVAL_FAILED;

    spdlog::info("-----邻域计算完成-----");
    
    return pRPL::EVAL_SUCCEEDED;
}

/*pRPL*/
void pRPL::Transition::
    addInputLyr(const char *aInLyrName,
                bool isPrimeLyr)
{
    if (aInLyrName != NULL &&
        std::find(_vInLyrNames.begin(), _vInLyrNames.end(), aInLyrName) == _vInLyrNames.end())
    {
        _vInLyrNames.push_back(aInLyrName);
        if (_mpCellspcs.find(aInLyrName) == _mpCellspcs.end())
        {
            _mpCellspcs[aInLyrName] = NULL;
        }
        if (isPrimeLyr)
        {
            _primeLyrName = aInLyrName;
        }
    }
}

void pRPL::Transition::
    addOutputLyr(const char *aOutLyrName,
                 bool isPrimeLyr)
{
    if (aOutLyrName != NULL &&
        std::find(_vOutLyrNames.begin(), _vOutLyrNames.end(), aOutLyrName) == _vOutLyrNames.end())
    {
        _vOutLyrNames.push_back(aOutLyrName);
        if (_mpCellspcs.find(aOutLyrName) == _mpCellspcs.end())
        {
            _mpCellspcs[aOutLyrName] = NULL;
        }
        if (isPrimeLyr)
        {
            _primeLyrName = aOutLyrName;
        }
    }
}

bool pRPL::Transition::
    setLyrsByNames(vector<string> *pvInLyrNames,
                   vector<string> *pvOutLyrNames,
                   string &primeLyrName)
{
    clearLyrSettings();

    vector<string>::iterator itrLyrName;
    bool foundPrm = false;
    if (pvInLyrNames != NULL)
    {
        itrLyrName = std::find(pvInLyrNames->begin(), pvInLyrNames->end(), primeLyrName);
        if (itrLyrName != pvInLyrNames->end())
        {
            _primeLyrName = primeLyrName;
            foundPrm = true;
        }
        _vInLyrNames = *pvInLyrNames;
    }
    if (pvOutLyrNames != NULL)
    {
        itrLyrName = find(pvOutLyrNames->begin(), pvOutLyrNames->end(), primeLyrName);
        if (itrLyrName != pvOutLyrNames->end())
        {
            _primeLyrName = primeLyrName;
            foundPrm = true;
        }
        _vOutLyrNames = *pvOutLyrNames;
    }
    if (!foundPrm)
    {
        cerr << __FILE__ << " function: " << __FUNCTION__
             << " Error: primary Layer name (" << primeLyrName
             << ") is not found in either Input Layer names or Output Layer names" << endl;
        return false;
    }

    for (int iLyrName = 0; iLyrName < _vInLyrNames.size(); iLyrName++)
    {
        const string &lyrName = _vInLyrNames[iLyrName];
        _mpCellspcs[lyrName] = NULL;
    }
    for (int iLyrName = 0; iLyrName < _vOutLyrNames.size(); iLyrName++)
    {
        const string &lyrName = _vOutLyrNames[iLyrName];
        if (std::find(_vInLyrNames.begin(), _vInLyrNames.end(), lyrName) != _vInLyrNames.end())
        {
            /*
            cerr << __FILE__ << " function: " << __FUNCTION__ \
                 << " Error: output Layer (" << lyrName \
                 << ") can NOT be also input Layer" << endl;
            return false;
            */
            continue; // ignore the output Layer that is also an input Layer
        }
        _mpCellspcs[lyrName] = NULL;
    }

    return true;
}

const vector<string> &pRPL::Transition::
    getInLyrNames() const
{
    return _vInLyrNames;
}

const vector<string> &pRPL::Transition::
    getOutLyrNames() const
{
    return _vOutLyrNames;
}

const string &pRPL::Transition::
    getPrimeLyrName() const
{
    return _primeLyrName;
}

bool pRPL::Transition::
    isInLyr(const string &lyrName) const
{
    return (std::find(_vInLyrNames.begin(), _vInLyrNames.end(), lyrName) != _vInLyrNames.end());
}

bool pRPL::Transition::
    isOutLyr(const string &lyrName) const
{
    return (std::find(_vOutLyrNames.begin(), _vOutLyrNames.end(), lyrName) != _vOutLyrNames.end());
}

bool pRPL::Transition::
    isPrimeLyr(const string &lyrName) const
{
    return (lyrName == _primeLyrName);
}

void pRPL::Transition::
    clearLyrSettings()
{
    _vInLyrNames.clear();
    _vOutLyrNames.clear();
    _primeLyrName.clear();
    _mpCellspcs.clear();
}

bool pRPL::Transition::
    setCellspace(const string &lyrName,
                 pRPL::Cellspace *pCellspc)
{
    map<string, pRPL::Cellspace *>::iterator itrCellspc = _mpCellspcs.find(lyrName);
    if (itrCellspc == _mpCellspcs.end())
    {
        cerr << __FILE__ << " function: " << __FUNCTION__
             << " Error: unable to find a Layer with the name ("
             << lyrName << ")" << endl;
        return false;
    }

    if (pCellspc == NULL ||
        pCellspc->isEmpty(true))
    {
        cerr << __FILE__ << " function: " << __FUNCTION__
             << " Error: NULL or empty Cellspace to be added to the Transition"
             << endl;
        return false;
    }
    itrCellspc->second = pCellspc;

    return true;
}

void pRPL::Transition::
    clearCellspaces()
{
    map<string, pRPL::Cellspace *>::iterator itrCellspc = _mpCellspcs.begin();
    while (itrCellspc != _mpCellspcs.end())
    {
        itrCellspc->second = NULL;
        itrCellspc++;
    }
}

pRPL::Cellspace *pRPL::Transition::
    getCellspaceByLyrName(const string &lyrName)
{
    pRPL::Cellspace *pCellspc = NULL;
    map<string, pRPL::Cellspace *>::iterator itrCellspc = _mpCellspcs.find(lyrName);
    if (itrCellspc != _mpCellspcs.end())
    {
        pCellspc = itrCellspc->second;
    }
    return pCellspc;
}

const pRPL::Cellspace *pRPL::Transition::
    getCellspaceByLyrName(const string &lyrName) const
{
    const pRPL::Cellspace *pCellspc = NULL;
    map<string, pRPL::Cellspace *>::const_iterator itrCellspc = _mpCellspcs.find(lyrName);
    if (itrCellspc != _mpCellspcs.end())
    {
        pCellspc = itrCellspc->second;
    }
    return pCellspc;
}

void pRPL::Transition::
    setUpdateTracking(bool toTrack)
{
    for (int iOutLyrName = 0; iOutLyrName < _vOutLyrNames.size(); iOutLyrName++)
    {
        map<string, Cellspace *>::iterator itrCellspc = _mpCellspcs.find(_vOutLyrNames[iOutLyrName]);
        if (itrCellspc != _mpCellspcs.end() &&
            itrCellspc->second != NULL)
        {
            itrCellspc->second->setUpdateTracking(toTrack);
        }
    }
}

void pRPL::Transition::
    clearUpdateTracks()
{
    for (int iOutLyrName = 0; iOutLyrName < _vOutLyrNames.size(); iOutLyrName++)
    {
        map<string, Cellspace *>::iterator itrCellspc = _mpCellspcs.find(_vOutLyrNames[iOutLyrName]);
        if (itrCellspc != _mpCellspcs.end() &&
            itrCellspc->second != NULL)
        {
            itrCellspc->second->clearUpdatedIdxs();
        }
    }
}

void pRPL::Transition::
    setNbrhdByName(const char *aNbrhdName)
{
    if (aNbrhdName != NULL)
    {
        _nbrhdName = aNbrhdName;
    }
}

const string &pRPL::Transition::
    getNbrhdName() const
{
    return _nbrhdName;
}

void pRPL::Transition::
    clearNbrhdSetting()
{
    _nbrhdName.clear();
    _pNbrhd = NULL;
}

void pRPL::Transition::
    setNbrhd(Neighborhood *pNbrhd)
{
    _pNbrhd = pNbrhd;
}

pRPL::Neighborhood *pRPL::Transition::
    getNbrhd()
{
    return _pNbrhd;
}

const pRPL::Neighborhood *pRPL::Transition::
    getNbrhd() const
{
    return _pNbrhd;
}

void pRPL::Transition::
    clearDataSettings()
{
    clearLyrSettings();
    clearNbrhdSetting();
}

bool pRPL::Transition::
    onlyUpdtCtrCell() const
{
    return _onlyUpdtCtrCell;
}

bool pRPL::Transition::
    needExchange() const
{
    return _needExchange;
}

bool pRPL::Transition::
    edgesFirst() const
{
    return _edgesFirst;
}

pRPL::EvaluateReturn pRPL::Transition::
    evalBR(const pRPL::CoordBR &br, bool GPUCompute, pRPL::pCuf pf)
{
    pRPL::Cellspace *pPrmSpc = getCellspaceByLyrName(getPrimeLyrName());

    if (pPrmSpc == NULL)
    {
        cerr << __FILE__ << " function: " << __FUNCTION__
             << " Error: unable to find the primary Cellspace with name ("
             << getPrimeLyrName() << ")" << endl;
        return pRPL::EVAL_FAILED;
    }
    pRPL::EvaluateReturn done = pRPL::EVAL_SUCCEEDED;

    clock_t start_time, end_time;
    start_time = clock();
    if (br.valid(pPrmSpc->info()->dims()))
    {
        /* CPU计算 */
        if (!GPUCompute)
        {
            clock_t start, end;
            start = clock();
            for (long iRow = br.minIRow(); iRow <= br.maxIRow(); iRow++)
            {
                for (long iCol = br.minICol(); iCol <= br.maxICol(); iCol++)
                {

                    done = evaluate(pRPL::CellCoord(iRow, iCol));
                    if (done == pRPL::EVAL_FAILED ||
                        done == pRPL::EVAL_TERMINATED)
                    {
                        return done;
                    }
                }
            }
            end = clock();
            cout << "time : " << double(end - start) / 1000 << "ms." << endl;
        }
        else
        { /* GPU计算 */
            (this->*pf)(br);
        }
    }
    end_time = clock();

    return pRPL::EVAL_SUCCEEDED;
}

pRPL::EvaluateReturn pRPL::Transition::
    evalRandomly(const pRPL::CoordBR &br)
{
    pRPL::Cellspace *pPrmSpc = getCellspaceByLyrName(getPrimeLyrName());
    if (pPrmSpc == NULL)
    {
        cerr << __FILE__ << " function: " << __FUNCTION__
             << " Error: unable to find the primary Cellspace with name ("
             << getPrimeLyrName() << ")" << endl;
        return pRPL::EVAL_FAILED;
    }

    pRPL::EvaluateReturn done = pRPL::EVAL_SUCCEEDED;
    if (br.valid(pPrmSpc->info()->dims()))
    {
        while (done != pRPL::EVAL_TERMINATED && done != pRPL::EVAL_FAILED)
        {
            long iRow = rand() % br.nRows() + br.minIRow();
            long iCol = rand() % br.nCols() + br.minICol();
            pRPL::CellCoord coord2Eval(iRow, iCol);
            if (br.ifContain(coord2Eval))
            {
                done = evaluate(coord2Eval);
            }
        }
    }

    return done;
}

pRPL::EvaluateReturn pRPL::Transition::
    evalSelected(const pRPL::CoordBR &br,
                 const pRPL::LongVect &vlclIdxs)
{
    pRPL::Cellspace *pPrmSpc = getCellspaceByLyrName(getPrimeLyrName());
    if (pPrmSpc == NULL)
    {
        cerr << __FILE__ << " function: " << __FUNCTION__
             << " Error: unable to find the primary Cellspace with name ("
             << getPrimeLyrName() << ")" << endl;
        return pRPL::EVAL_FAILED;
    }

    pRPL::EvaluateReturn done = pRPL::EVAL_SUCCEEDED;
    if (br.valid(pPrmSpc->info()->dims()))
    {
        for (int iIdx = 0; iIdx < vlclIdxs.size(); iIdx++)
        {
            pRPL::CellCoord coord = pPrmSpc->info()->idx2coord(vlclIdxs[iIdx]);
            if (br.ifContain(coord))
            {
                done = evaluate(coord);
                if (done == pRPL::EVAL_FAILED ||
                    done == pRPL::EVAL_TERMINATED)
                {
                    return done;
                }
            }
        }
    }

    return pRPL::EVAL_SUCCEEDED;
}

bool pRPL::Transition::
    afterSetCellspaces(int subCellspcGlbIdx)
{
    return true;
}

bool pRPL::Transition::
    afterSetNbrhd()
{
    return true;
}

bool pRPL::Transition::
    check() const
{
    return true;
}

pRPL::EvaluateReturn pRPL::Transition::
    evaluate(const CellCoord &coord)
{
    return pRPL::EVAL_SUCCEEDED;
}

double pRPL::Transition::
    workload(const CoordBR &workBR) const
{
    return 0.0;
}
