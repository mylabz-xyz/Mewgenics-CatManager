#pragma once

#include <cstdint>

struct PedigreeNode
{
    int64_t id = 0;
    int64_t parentA = -1;
    int64_t parentB = -1;
    double coi = 0.0;
};