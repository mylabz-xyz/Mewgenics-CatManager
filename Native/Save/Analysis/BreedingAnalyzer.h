#pragma once

#include "../Model/SaveData.h"

#include <cstddef>
#include <cstdint>
#include <vector>

struct BreedingCommonAncestor
{
    const CatData *ancestor = nullptr;
    size_t depthA = 0U;
    size_t depthB = 0U;
    double ancestorCoi = 0.0;
    double contribution = 0.0;
};

struct BreedingAnalysis
{
    const CatData *catA = nullptr;
    const CatData *catB = nullptr;

    bool valid = false;

    std::vector<BreedingCommonAncestor> commonAncestors;

    double expectedOffspringCoi = 0.0;
};

BreedingAnalysis AnalyzeBreeding(
    const SaveData &save,
    const CatData &catA,
    const CatData &catB,
    size_t maxDepth);