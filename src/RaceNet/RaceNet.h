//
// Created by Amrik on 28/10/2018.
//

#pragma once

#include <iostream>
#include <time.h>
#include <stdlib.h>

#include "../Util/Logger.h"
#include "Network.h"

static const std::vector<int> layerParams{2, 3, 2};
// 2 input neurons (i.e. 2 inputs)
// 3 hidden neurons (experimental) : you can specify as many hidden layers as you want (you need to add the number of neurons in each)
// 2 output neurons (2 outputs)
// 0.7 learning rate (experimental)

class RaceNet {
private:
    Network net;
public:
    explicit RaceNet();
    std::vector<double> Infer(std::vector<double> raycastInputs);
};

