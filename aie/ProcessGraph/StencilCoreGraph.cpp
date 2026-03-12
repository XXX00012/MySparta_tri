#include "../ProcessGraph/StencilCoreGraph.h"

StencilCoreGraph::StencilCoreGraph() {
    k_lap   = kernel::create(hdiff_lap);
    k_flux1 = kernel::create(hdiff_flux1);
    k_flux2 = kernel::create(hdiff_flux2);

    source(k_lap)   = "ProcessUnit/hdiff_lap.cc";
    source(k_flux1) = "ProcessUnit/hdiff_flux1.cc";
    source(k_flux2) = "ProcessUnit/hdiff_flux2.cc";

    headers(k_lap)   = {"ProcessUnit/hdiff.h", "ProcessUnit/include.h"};
    headers(k_flux1) = {"ProcessUnit/hdiff.h", "ProcessUnit/include.h"};
    headers(k_flux2) = {"ProcessUnit/hdiff.h", "ProcessUnit/include.h"};

    runtime<ratio>(k_lap)   = 0.9;
    runtime<ratio>(k_flux1) = 0.9;
    runtime<ratio>(k_flux2) = 0.9;

    location<kernel>(k_lap)   = tile(7, 1);
    location<kernel>(k_flux1) = tile(7, 2);
    location<kernel>(k_flux2) = tile(7, 3);

    connect<window<COL * NBYTES>>(in[0], k_lap.in[0]);
    connect<window<COL * NBYTES>>(in[1], k_lap.in[1]);
    connect<window<COL * NBYTES>>(in[2], k_lap.in[2]);
    connect<window<COL * NBYTES>>(in[3], k_lap.in[3]);
    connect<window<COL * NBYTES>>(in[4], k_lap.in[4]);

    connect<window<COL * NBYTES>>(in[1], k_flux1.in[0]);
    connect<window<COL * NBYTES>>(in[2], k_flux1.in[1]);
    connect<window<COL * NBYTES>>(in[3], k_flux1.in[2]);

    connect<window<COL * NBYTES>>(k_lap.out[0], k_flux1.in[3]);
    connect<window<COL * NBYTES>>(k_lap.out[1], k_flux1.in[4]);
    connect<window<COL * NBYTES>>(k_lap.out[2], k_flux1.in[5]);
    connect<window<COL * NBYTES>>(k_lap.out[3], k_flux1.in[6]);

    connect<window<2 * COL * NBYTES>>(k_flux1.out[0], k_flux2.in[0]);
    connect<window<2 * COL * NBYTES>>(k_flux1.out[1], k_flux2.in[1]);
    connect<window<2 * COL * NBYTES>>(k_flux1.out[2], k_flux2.in[2]);
    connect<window<2 * COL * NBYTES>>(k_flux1.out[3], k_flux2.in[3]);
    connect<window<2 * COL * NBYTES>>(k_flux1.out[4], k_flux2.in[4]);

    connect<window<COL * NBYTES>>(k_flux2.out[0], out);
}