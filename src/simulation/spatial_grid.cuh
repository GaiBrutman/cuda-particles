#pragma once

#include <cuda_runtime.h>

// Uniform spatial hash grid for O(n) neighbor queries.
// Cell size should be >= 2 * PARTICLE_RADIUS so each particle
// only needs to check the 3x3 neighborhood of cells.

constexpr float GRID_CELL_SIZE = 8.0f;  // pixels; covers 2 * PARTICLE_RADIUS

struct SpatialGrid {
    int* cellStart;   // first sorted index for each cell (-1 = empty)
    int* cellEnd;     // one-past-last sorted index for each cell
    int* sortedIdx;   // particle indices sorted by cell hash
    int* cellHash;    // cell hash for each particle (pre-sort)
    int  gridW;
    int  gridH;
    int  cellCount;
};

__device__ inline int2 worldToCellDevice(float x, float y) {
    return make_int2((int)(x / GRID_CELL_SIZE), (int)(y / GRID_CELL_SIZE));
}

__device__ inline int hashCellDevice(int cx, int cy, int gridW) {
    return cy * gridW + cx;
}
