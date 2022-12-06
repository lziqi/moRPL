#include "demoTrans.h"

DemoTransition::DemoTransition() : pRPL::Transition(), _inputLayer(NULL), _outputLayer(NULL), _scale(1.0)
{
    _needExchange = false;
    _edgesFirst = false;
}

DemoTransition::~DemoTransition()
{
    _inputLayer = NULL;
    _outputLayer = NULL;
}

bool DemoTransition::check() const
{
    if (_inputLayer == NULL)
    {
        cerr << __FILE__ << "InputLayer null" << endl;
        return false;
    }
    if (_outputLayer == NULL)
    {
        cerr << __FILE__ << "OutputLayer null" << endl;
    }
    return true;
}

bool DemoTransition::afterSetCellspaces(int subCellspcGlbIdx)
{
    _inputLayer = getCellspaceByLyrName(_vInLyrNames[0]);
    _limitLayer = getCellspaceByLyrName("limitLayer");
    _probLayer = getCellspaceByLyrName("probLayer");
    // cout << _inputLayer->info()->size() << endl;

    _outputLayer = getCellspaceByLyrName(_vOutLyrNames[0]);

    // cout << _outputLayer->info()->size() << endl;
    return true;
}

pRPL::EvaluateReturn DemoTransition::
    evaluate(const pRPL::CellCoord &coord)
{
    pRPL::IntVect vDemVals;
    // if (!_inputLayer->nbrhdValsAs<int>(vDemVals, *_pNbrhd, coord, true, true)) {
    //     return pRPL::EVAL_FAILED;
    // }
    // cout << _inputLayer->atAs<float>(coord) << endl;

    /* 输出图层数据 */
    unsigned char _data = _inputLayer->atAs<unsigned char>(coord, true);
    unsigned char _limit = _limitLayer->atAs<unsigned char>(coord, true);
    unsigned char _prob = _probLayer->atAs<unsigned char>(coord, true);

    int test[16] = {-1, -1, -1, 0, -1, 1, 0,
                    -1, 0, 1, 1, -1, 1, 0, 1, 1};

    if (_data > 1)
        return pRPL::EVAL_SUCCEEDED;

    unsigned char sum = 0;
    for (int i = 0; i < 8; i++) //不加自己所在的邻域
    {
        int row = coord.iRow() + test[i * 2];
        int col = coord.iCol() + test[i * 2 + 1];
        pRPL::CellCoord nbrCoord(row, col);

        // if (_inputLayer->atAs<unsigned char>(nbrCoord, true) != 0)
        sum += _inputLayer->atAs<unsigned char>(nbrCoord, true);
    }

    float averageData = sum * 1.0 / 8;
    float trans_prob = averageData * (float)_limit * float(_prob) / 100.0;

    float res = 0;
    if(trans_prob > 0.06)
        res = 1;
    else
        res = _data;

    if (!_outputLayer->updateCellAs<unsigned char>(coord, res, true))
    {
        return pRPL::EVAL_FAILED;
    }

    return pRPL::EVAL_SUCCEEDED;
}
