#pragma once
#include <adf.h>
#include <string>
#include "./ProcessUnit/include.h"
#include "./ProcessGraph/StencilCoreGraph.h"

using namespace adf;

class TopStencilGraph : public graph {
public:
    StencilCoreGraph core;
    input_gmio  in_gmio[5];
    output_gmio out_gmio;

    TopStencilGraph(const std::string& graphID);
};

extern TopStencilGraph topStencil;