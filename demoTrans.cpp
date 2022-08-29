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

    int test[18] = {0, 0, -1, -1, -1, 0, -1, 1, 0,
                    -1, 0, 1, 1, -1, 1, 0, 1, 1};

    unsigned char sum = 0;
    for (int i = 0; i < 9; i++)
    {
        int row = coord.iRow() + test[i * 2];
        int col = coord.iCol() + test[i * 2 + 1];
        pRPL::CellCoord nbrCoord(row, col);

        if (_inputLayer->atAs<unsigned char>(nbrCoord, true) != 0)
            sum += _inputLayer->atAs<unsigned char>(nbrCoord, true);
    }

    if (!_outputLayer->updateCellAs<unsigned char>(coord, sum, true))
    {
        return pRPL::EVAL_FAILED;
    }

    return pRPL::EVAL_SUCCEEDED;
}
