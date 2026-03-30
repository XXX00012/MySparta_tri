#pragma once

#include <adf.h>
#include "../ProcessUnit/include.h"
#include "../Config.h"
#include "../ProcessUnit/hdiff.h"

using namespace adf;

class StencilCoreGraph : public graph {
public:
    port<input>  in;
    port<output> out;

    kernel k_lap;
    kernel k_flux1;
    kernel k_flux2;

    StencilCoreGraph() {
#if defined(__AIESIM__) || defined(__X86SIM__) || defined(__ADF_FRONTEND__)
        k_lap   = kernel::create(hdiff_lap);
        k_flux1 = kernel::create(hdiff_flux1);
        k_flux2 = kernel::create(hdiff_flux2);

        source(k_lap)   = "ProcessUnit/hdiff_lap.cc";
        source(k_flux1) = "ProcessUnit/hdiff_flux1.cc";
        source(k_flux2) = "ProcessUnit/hdiff_flux2.cc";

        headers(k_lap)   = {"ProcessUnit/hdiff.h", "ProcessUnit/include.h", "Config.h"};
        headers(k_flux1) = {"ProcessUnit/hdiff.h", "ProcessUnit/include.h", "Config.h"};
        headers(k_flux2) = {"ProcessUnit/hdiff.h", "ProcessUnit/include.h", "Config.h"};

        runtime<ratio>(k_lap)   = 0.9;
        runtime<ratio>(k_flux1) = 0.9;
        runtime<ratio>(k_flux2) = 0.9;

        location<kernel>(k_lap)   = tile(hdiff_cfg::kTileCol, hdiff_cfg::kLapTileRow);
        location<kernel>(k_flux1) = tile(hdiff_cfg::kTileCol, hdiff_cfg::kFlux1TileRow);
        location<kernel>(k_flux2) = tile(hdiff_cfg::kTileCol, hdiff_cfg::kFlux2TileRow);

        // ------------------------------------------------------------
        // Single input broadcast to lap + flux1
        // NOTE:
        // Current kernel interfaces use input_buffer/output_buffer,
        // so graph connections must be plain connect(...), not
        // connect<adf::window<...>>.
        // ------------------------------------------------------------
        auto net_in_lap   = connect(in, k_lap.in[0]);
        auto net_in_flux1 = connect(in, k_flux1.in[0]);

        // Explicit dimensions are required for buffer ports.
        dimensions(k_lap.in[0])   = {hdiff_cfg::kWindowRows * COL};   // 5 * 256
        dimensions(k_flux1.in[0]) = {hdiff_cfg::kWindowRows * COL};   // 5 * 256

        fifo_depth(net_in_lap)   = hdiff_cfg::kInputObjectFifoDepth;  // 6
        fifo_depth(net_in_flux1) = hdiff_cfg::kInputObjectFifoDepth;  // 6

        // ------------------------------------------------------------
        // lap -> flux1 : 4 x 256
        // ------------------------------------------------------------
        auto net_lap_f1_0 = connect(k_lap.out[0], k_flux1.in[1]);
        auto net_lap_f1_1 = connect(k_lap.out[1], k_flux1.in[2]);
        auto net_lap_f1_2 = connect(k_lap.out[2], k_flux1.in[3]);
        auto net_lap_f1_3 = connect(k_lap.out[3], k_flux1.in[4]);

        dimensions(k_lap.out[0])  = {COL};
        dimensions(k_lap.out[1])  = {COL};
        dimensions(k_lap.out[2])  = {COL};
        dimensions(k_lap.out[3])  = {COL};

        dimensions(k_flux1.in[1]) = {COL};
        dimensions(k_flux1.in[2]) = {COL};
        dimensions(k_flux1.in[3]) = {COL};
        dimensions(k_flux1.in[4]) = {COL};

        fifo_depth(net_lap_f1_0) = hdiff_cfg::kLapObjectFifoDepth;    // 5
        fifo_depth(net_lap_f1_1) = hdiff_cfg::kLapObjectFifoDepth;    // 5
        fifo_depth(net_lap_f1_2) = hdiff_cfg::kLapObjectFifoDepth;    // 5
        fifo_depth(net_lap_f1_3) = hdiff_cfg::kLapObjectFifoDepth;    // 5

        // ------------------------------------------------------------
        // flux1 -> flux2 : 5 x 512
        // ------------------------------------------------------------
        auto net_f1_f2_0 = connect(k_flux1.out[0], k_flux2.in[0]);
        auto net_f1_f2_1 = connect(k_flux1.out[1], k_flux2.in[1]);
        auto net_f1_f2_2 = connect(k_flux1.out[2], k_flux2.in[2]);
        auto net_f1_f2_3 = connect(k_flux1.out[3], k_flux2.in[3]);
        auto net_f1_f2_4 = connect(k_flux1.out[4], k_flux2.in[4]);

        dimensions(k_flux1.out[0]) = {2 * COL};
        dimensions(k_flux1.out[1]) = {2 * COL};
        dimensions(k_flux1.out[2]) = {2 * COL};
        dimensions(k_flux1.out[3]) = {2 * COL};
        dimensions(k_flux1.out[4]) = {2 * COL};

        dimensions(k_flux2.in[0])  = {2 * COL};
        dimensions(k_flux2.in[1])  = {2 * COL};
        dimensions(k_flux2.in[2])  = {2 * COL};
        dimensions(k_flux2.in[3])  = {2 * COL};
        dimensions(k_flux2.in[4])  = {2 * COL};

        fifo_depth(net_f1_f2_0) = hdiff_cfg::kFluxInterObjectFifoDepth; // 6
        fifo_depth(net_f1_f2_1) = hdiff_cfg::kFluxInterObjectFifoDepth; // 6
        fifo_depth(net_f1_f2_2) = hdiff_cfg::kFluxInterObjectFifoDepth; // 6
        fifo_depth(net_f1_f2_3) = hdiff_cfg::kFluxInterObjectFifoDepth; // 6
        fifo_depth(net_f1_f2_4) = hdiff_cfg::kFluxInterObjectFifoDepth; // 6

        // ------------------------------------------------------------
        // flux2 -> out : 256
        // ------------------------------------------------------------
        auto net_out = connect(k_flux2.out[0], out);

        dimensions(k_flux2.out[0]) = {COL};

        fifo_depth(net_out) = hdiff_cfg::kOutputObjectFifoDepth;      // 2
#endif
    }
};