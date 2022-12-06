#include "prpl-transition.h"
#include <cmath>
#include <functional>

using namespace std;

class DemoTransition : public pRPL::Transition {
public:
    DemoTransition();
    ~DemoTransition();

    bool check()const;
    bool afterSetCellspaces(int subCellspcGlbIdx = pRPL::ERROR_ID);
    pRPL::EvaluateReturn evaluate(const pRPL::CellCoord &coord);

protected:
    float _scale;

    pRPL::Cellspace *_inputLayer;
    pRPL::Cellspace *_inLayer;
    pRPL::Cellspace *_limitLayer;
    pRPL::Cellspace *_probLayer;

    pRPL::Cellspace *_outputLayer;
};
