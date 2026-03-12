#include "TopGraph.h"

TopStencilGraph::TopStencilGraph(const std::string& graphID) {
    for (int i = 0; i < 5; ++i) {
        in_gmio[i] = input_gmio::create(
            graphID + "_gmio_in" + std::to_string(i),
            64,
            1000
        );
    }

    out_gmio = output_gmio::create(
        graphID + "_gmio_out0",
        64,
        1000
    );

    connect<window<COL * NBYTES>>(in_gmio[0].out[0], core.in[0]);
    connect<window<COL * NBYTES>>(in_gmio[1].out[0], core.in[1]);
    connect<window<COL * NBYTES>>(in_gmio[2].out[0], core.in[2]);
    connect<window<COL * NBYTES>>(in_gmio[3].out[0], core.in[3]);
    connect<window<COL * NBYTES>>(in_gmio[4].out[0], core.in[4]);

    connect<window<COL * NBYTES>>(core.out, out_gmio.in[0]);
}

TopStencilGraph topStencil("hdiff");

#ifndef __X86SIM__
#ifndef __AIESIM__
int main() {
    topStencil.init();
    topStencil.run(2);
    topStencil.end();
    return 0;
}
#endif
#endif