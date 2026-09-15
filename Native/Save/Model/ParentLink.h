#pragma once

#include <cstdint>


struct ParentLink
{
    uint64_t child;
    uint64_t parentA;
    uint64_t parentB;
};