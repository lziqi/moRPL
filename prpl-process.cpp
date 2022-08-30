#include "prpl-process.h"

/* moRPL */
moRPL::Node pRPL::Process::getNode()
{
  return this->node;
}

bool pRPL::Process::initOpenCL()
{
  MPI_Status status;

  if (!this->node.valid() && !isMaster()) //非主节点且没有GPU
    return false;
  else
  {
    if (this->isMaster()) //主进程
    {
      map<string, vector<int>> nodeCpu;          //节点对应CPU
      map<int, string> processNode;              //进程对应节点名
      map<int, vector<cl_device_id>> processGPU; //进程对应GPU

      nodeCpu[_prcrName] = vector<int>(1, 0);
      processNode[_id] = _prcrName;
      processGPU[_id] = node.getGPUIDs();

      /* 生成节点 --- CPU、GPU对应表 */
      for (int i = 1; i < _nTotalPrcs; i++)
      {
        char _name[100];
        string name;
        int gpuCount;

        MPI_Recv(_name, 100, MPI_CHAR, i, i, _comm, &status);
        MPI_Recv(&gpuCount, 1, MPI_INT, i, 2 * i, _comm, &status);
        vector<cl_device_id> gpuIDs(gpuCount);
        MPI_Recv(&gpuIDs[0], gpuCount, MPI_DOUBLE, i, 3 * i, _comm, &status);

        processNode[i] = _name;
        processGPU[i] = gpuIDs;

        /* 初始化 节点与CPU的对应表 */
        name = _name;
        auto it = nodeCpu.find(name);
        /* 节点名首次出现 */
        if (it == nodeCpu.end())
        {
          nodeCpu[name] = vector<int>(1, i);
        }
        else
          it->second.push_back(i);
      }

      /* 向每个进程分配GPU */
      for (auto &i : processGPU)
      {
        int processID = i.first;
        vector<cl_device_id> gpuIDs = i.second;

        /* 当前进程所在节点的第一个进程ID */
        string nodeName = processNode[processID];
        int processBegin = nodeCpu[nodeName][0];
        int gpuNum = gpuIDs.size();

        cl_device_id gpuID = gpuIDs[(processID - processBegin) % gpuNum];

        if (processID == 0) //主进程
        {
          node.setGPUID(gpuID);
        }
        else //其他进程
        {
          MPI_Send(&gpuID, 1, MPI_DOUBLE, processID, 4 * processID, _comm);
        }
      }

      // for (auto &i : nodeCpu)
      // {
      //   vector<int> cpus = i.second; //当前节点对应的CPU进程ID

      //   for (int k = 0; k < cpus.size(); k++)
      //   {
      //     int gpuIndex = k % gpus.size();
      //     if (cpus[k] != 0)
      //     {
      //       MPI_Send(&gpus[gpuIndex], 1, MPI_DOUBLE, cpus[k], 4 * cpus[k], _comm);
      //     }
      //     else //主进程
      //     {
      //       node.setGPUID(gpus[gpuIndex]);
      //       spdlog::info("主进程GPUID : ");
      //       cout << gpus[gpuIndex] << endl;
      //     }
      //   }
      // }
    }
    else //非主进程
    {
      /* 发送节点名 */
      const char *name = _prcrName.c_str();
      MPI_Send(name, 100, MPI_CHAR, 0, _id, _comm);

      /* 发送GPU数 */
      int gpuCount = node.getGPUIDs().size();
      MPI_Send(&gpuCount, 1, MPI_INT, 0, 2 * _id, _comm);

      /* 发送GPUIDs */
      MPI_Send(&node.getGPUIDs()[0], gpuCount, MPI_DOUBLE, 0, 3 * _id, _comm); // GPU0-n发送给主节点

      /* 接受GPUID */
      cl_device_id gpuID;
      MPI_Recv(&gpuID, 1, MPI_DOUBLE, 0, 4 * _id, _comm, &status);

      node.setGPUID(gpuID);
    }
  }

  return true;
}

bool pRPL::Process::initOpenCL_Old()
{
  MPI_Status status;
  if (!this->node.valid() && !isMaster()) //非主节点且没有GPU
    return false;
  else
  {
    if (this->isMaster())
    {                                            //主节点
      map<string, vector<int>> nodeCpu;          //节点对应CPU
      map<string, vector<cl_device_id>> nodeGpu; //节点对应GPU

      map<string, vector<int>>::iterator it;

      nodeCpu[_prcrName] = vector<int>(1, 0);
      nodeGpu[_prcrName] = node.getGPUIDs();

      /* 生成节点 --- CPU、GPU对应表 */
      for (int i = 1; i < _nTotalPrcs; i++)
      {
        char _name[100];
        string name;
        int gpuCount;

        MPI_Recv(_name, 100, MPI_CHAR, i, i, _comm, &status);
        MPI_Recv(&gpuCount, 1, MPI_INT, i, 2 * i, _comm, &status);

        vector<cl_device_id> gpuIDs(gpuCount);

        MPI_Recv(&gpuIDs[0], gpuCount, MPI_DOUBLE, i, 3 * i, _comm, &status);
        spdlog::info("0进程 接收到 {}进程 的GPUID : ", i);
        for (int i = 0; i < gpuIDs.size(); i++)
          cout << gpuIDs[i] << endl;

        name = _name;

        /* 初始化 节点与CPU的对应表 */
        it = nodeCpu.find(name);
        /* 节点名首次出现 */
        if (it == nodeCpu.end())
        {
          nodeCpu[name] = vector<int>(1, i);
          nodeGpu[name] = gpuIDs;
        }
        else
          it->second.push_back(i);
      }

      spdlog::info("GPU对应表:");
      for (auto &i : nodeGpu)
      {
        cout << i.first << " " << i.second[0] << endl;
      }

      /* 为所有节点，向CPU分配GPU */
      // map<string, vector<int>>::iterator i;
      // map<string, vector<cl_device_id>>::iterator j;
      for (auto &i : nodeCpu)
      {
        auto j = nodeGpu.begin();
        vector<int> cpus = i.second; //当前节点对应的CPU进程ID
        vector<cl_device_id> gpus = j->second;

        for (int k = 0; k < cpus.size(); k++)
        {
          int gpuIndex = k % gpus.size();
          if (cpus[k] != 0)
          {
            MPI_Send(&gpus[gpuIndex], 1, MPI_DOUBLE, cpus[k], 4 * cpus[k], _comm);
          }
          else //主进程
          {
            node.setGPUID(gpus[gpuIndex]);
            spdlog::info("主进程GPUID : ");
            cout << gpus[gpuIndex] << endl;
          }
        }
      }
    }
    else
    {
      const char *name = _prcrName.c_str();
      MPI_Send(name, 100, MPI_CHAR, 0, _id, _comm); //节点名发送给主节点
      int gpuCount = node.getGPUIDs().size();
      MPI_Send(&gpuCount, 1, MPI_INT, 0, 2 * _id, _comm); // GPU数发给主节点

      spdlog::info("{}进程 发送的GPUID:", _id);
      for (int i = 0; i < node.getGPUIDs().size(); i++)
        cout << node.getGPUIDs()[i] << endl;

      MPI_Send(&node.getGPUIDs()[0], gpuCount, MPI_DOUBLE, 0, 3 * _id, _comm); // GPU0-n发送给主节点

      cl_device_id gpuID;
      MPI_Recv(&gpuID, 1, MPI_DOUBLE, 0, 4 * _id, _comm, &status);

      node.setGPUID(gpuID);
      spdlog::info("其他进程GPUID : ");
      cout << gpuID << endl;
    }
  }
  return true;
}

pRPL::Process::
    Process()
    : _comm(MPI_COMM_NULL),
      _id(-1),
      _grpID(-1),
      _nTotalPrcs(-1),
      _hasWriter(false) {}

void pRPL::Process::
    abort() const
{
  MPI_Abort(_comm, -1);
}

void pRPL::Process::
    finalize() const
{
  MPI_Finalize();
}

void pRPL::Process::
    sync() const
{
  MPI_Barrier(_comm);
}

const MPI_Comm &pRPL::Process::
    comm() const
{
  return _comm;
}

int pRPL::Process::
    id() const
{
  return _id;
}

const char *pRPL::Process::
    processorName() const
{
  return _prcrName.c_str();
}

int pRPL::Process::
    groupID() const
{
  return _grpID;
}

int pRPL::Process::
    nProcesses() const
{
  return _nTotalPrcs;
}

bool pRPL::Process::
    hasWriter() const
{
  return _hasWriter;
}

bool pRPL::Process::
    isMaster() const
{
  return (_id == 0);
}

bool pRPL::Process::
    isWriter() const
{
  return (_hasWriter && _id == 1);
}

const pRPL::IntVect pRPL::Process::
    allPrcIDs(bool incldMaster,
              bool incldWriter) const
{
  pRPL::IntVect vPrcIDs;
  for (int iPrc = 0; iPrc < _nTotalPrcs; iPrc++)
  {
    if (iPrc == 0 && !incldMaster)
    {
      continue;
    }
    if (_hasWriter && iPrc == 1 && !incldWriter)
    {
      continue;
    }
    vPrcIDs.push_back(iPrc);
  }
  return vPrcIDs;
}

bool pRPL::Process::
    initialized() const
{
  int mpiStarted;
  MPI_Initialized(&mpiStarted);
  return static_cast<bool>(mpiStarted);
}

bool pRPL::Process::
    active() const
{
  return (_comm != MPI_COMM_NULL &&
          _id != -1 &&
          _nTotalPrcs != -1);
}

bool pRPL::Process::
    init(int argc,
         char *argv[])
{
  bool done = true;
  if (_comm == MPI_COMM_NULL)
  {
    return done;
  }

  if (!initialized())
  {
    MPI_Init(&argc, &argv);
  }
  MPI_Comm_rank(_comm, &_id);
  MPI_Comm_size(_comm, &_nTotalPrcs);

  char aPrcrName[MPI_MAX_PROCESSOR_NAME];
  int prcrNameLen;
  MPI_Get_processor_name(aPrcrName, &prcrNameLen);
  _prcrName.assign(aPrcrName, aPrcrName + prcrNameLen);

  if (_nTotalPrcs == 2 && _hasWriter)
  {
    cerr << __FILE__ << " " << __FUNCTION__
         << " Warning: only TWO Processes have been initialized,"
         << " including a master Process and a writer Process."
         << " The writer Process will NOT participate in the computing when Task Farming."
         << " And the writer Process will NOT participate in the computing in all cases."
         << endl;
  }
  else if (_nTotalPrcs == 1)
  {
    cerr << __FILE__ << " " << __FUNCTION__
         << " Warning: only ONE Process has been initialized,"
         << " including a master Process."
         << " The master Process will NOT participate in the computing when Task Farming."
         << endl;
    if (_hasWriter)
    {
      cerr << __FILE__ << " " << __FUNCTION__
           << " Error: unable to initialize an writer Process"
           << endl;
      done = false;
    }
  }

  /* 为每个node设置节点名 */
  this->node.setName(_prcrName);
  return (done && this->node.init() && this->initOpenCL());
}

bool pRPL::Process::
    set(MPI_Comm &comm,
        bool hasWriter,
        int groupID)
{
  _comm = comm;
  _hasWriter = hasWriter;
  _grpID = groupID;
  return (init());
}

bool pRPL::Process::
    grouping(int nGroups,
             bool incldMaster,
             Process *pGrpedPrc,
             Process *pGrpMaster) const
{
  if (!initialized())
  {
    cerr << __FILE__ << " " << __FUNCTION__
         << " Error: Process has NOT been initialized,"
         << " unable to be grouped" << endl;
    return false;
  }

  if (!active())
  {
    cerr << __FILE__ << " " << __FUNCTION__
         << " Error: inactive Process,"
         << " unable to group a Null communicator."
         << " id = " << _id << " nTotPrcs = " << _nTotalPrcs << endl;
    return false;
  }

  if (nGroups <= 0 ||
      nGroups > _nTotalPrcs)
  {
    cerr << __FILE__ << " " << __FUNCTION__
         << " Error: invalid number of groups ("
         << nGroups << ") as the total number of processes is "
         << _nTotalPrcs << endl;
    return false;
  }

  if (!incldMaster && _nTotalPrcs <= 1)
  {
    cerr << __FILE__ << " " << __FUNCTION__
         << " Error:  " << _nTotalPrcs << " processes can NOT"
         << " be grouped without the master process" << endl;
    return false;
  }

  MPI_Group glbGrp;
  MPI_Comm glbComm = _comm;
  MPI_Comm_group(glbComm, &glbGrp);
  int myID = -1;
  int grpID = -1;
  MPI_Comm grpComm = MPI_COMM_NULL;

  if (incldMaster)
  {
    myID = _id;
    grpID = myID % nGroups;
    MPI_Comm_split(glbComm, grpID, myID, &grpComm);
    if (!pGrpedPrc->set(grpComm, _hasWriter, grpID))
    {
      return false;
    }
    if (pGrpMaster != NULL)
    {
      MPI_Group masterGrp = MPI_GROUP_NULL;
      MPI_Comm masterComm = MPI_COMM_NULL;
      int grpMasterRange[1][3] = {{0, nGroups - 1, 1}};
      MPI_Group_range_incl(glbGrp, 1, grpMasterRange, &masterGrp);
      MPI_Comm_create(glbComm, masterGrp, &masterComm);
      if (!pGrpMaster->set(masterComm))
      {
        return false;
      }
    }
  }
  else
  {
    int excldRanks[1] = {0};
    MPI_Group glbGrp2 = MPI_GROUP_NULL;
    MPI_Group_excl(glbGrp, 1, excldRanks, &glbGrp2);
    MPI_Comm_create(_comm, glbGrp2, &glbComm);
    glbGrp = glbGrp2;
    if (!isMaster())
    {
      MPI_Comm_rank(glbComm, &myID);
      grpID = myID % nGroups;
      MPI_Comm_split(glbComm, grpID, myID, &grpComm);
      if (!pGrpedPrc->set(grpComm, _hasWriter, grpID))
      {
        return false;
      }
      if (pGrpMaster != NULL)
      {
        MPI_Group masterGrp = MPI_GROUP_NULL;
        MPI_Comm masterComm = MPI_COMM_NULL;
        int grpMasterRange[1][3] = {{0, nGroups - 1, 1}};
        MPI_Group_range_incl(glbGrp, 1, grpMasterRange, &masterGrp);
        MPI_Comm_create(glbComm, masterGrp, &masterComm);
        if (!pGrpMaster->set(masterComm))
        {
          return false;
        }
      }
    }
  }

  return true;
}
