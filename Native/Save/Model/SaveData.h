#pragma once

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cstdint>

#include "CatData.h"
#include "ParentLink.h"
#include "PedigreeNode.h"

struct RelationshipEntry
{
    int64_t catA = 0;
    int64_t catB = 0;
    double relationship = 0.0;
};

struct SaveData
{
    std::unordered_map<uint64_t, CatData> cats;
    std::vector<ParentLink> familyLinks;
    std::unordered_map<int64_t, PedigreeNode> pedigree;
    std::unordered_map<uint64_t, uint64_t> sqlToCat;
    std::unordered_set<int64_t> activeCats;
    std::vector<RelationshipEntry> relationships;
};