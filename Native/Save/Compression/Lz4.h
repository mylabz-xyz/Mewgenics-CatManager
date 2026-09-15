#pragma once

#include <cstddef>


bool Lz4DecompressBlock(
    const unsigned char* src,
    size_t srcSize,
    unsigned char* dst,
    size_t dstSize
);