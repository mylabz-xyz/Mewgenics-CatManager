#pragma once

#include <vector>
#include <string>
#include "../Model/CatData.h"

bool DecompressCatBlob(
    const std::vector<unsigned char>& input,
    std::vector<unsigned char>& output
);

CatData DecodeCat(const std::vector<uint8_t>& data);