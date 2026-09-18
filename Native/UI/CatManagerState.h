#pragma once

#include "../Save/Model/SaveData.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

enum class CatManagerView
{
    Closed,
    Search,
    Breeding
};

struct CatManagerState
{
    const SaveData *saveData = nullptr;

    std::vector<uint64_t> catOrder;
    size_t selectedCatIndex = 0U;

    std::string searchQuery;
    std::vector<uint64_t> searchResults;
    size_t selectedSearchIndex = 0U;

    uint64_t breedingCatAId = 0U;
    uint64_t breedingCatBId = 0U;

    CatManagerView currentView =
        CatManagerView::Closed;
};