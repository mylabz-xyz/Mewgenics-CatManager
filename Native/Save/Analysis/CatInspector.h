#pragma once

#include "../Model/SaveData.h"

enum class CatRelationshipType
{
    Unrelated,
    Parent,
    Child,
    FullSibling,
    HalfSibling,
    Other
};

struct CatPopulationStats
{
    size_t total = 0U;
    size_t living = 0U;
    size_t deceased = 0U;
};

// Parent A / Parent B are kept as stored in the save.
// They must not be interpreted as father/mother because Mewgenics
// allows parent combinations where that distinction is not reliable.
struct CatInspection
{
    CatPopulationStats population;

    const CatData *parentA = nullptr;
    const CatData *parentB = nullptr;

    std::vector<const CatData *> children;

    double coi = 0.0;
};

struct CatAncestor
{
    const CatData *cat = nullptr;
    size_t depth = 0U;
};

struct CommonAncestor
{
    const CatData *cat = nullptr;
    size_t depthA = 0U;
    size_t depthB = 0U;
};

struct CatRelationship
{
    CatRelationshipType type =
        CatRelationshipType::Unrelated;

    bool isParent = false;
    bool isChild = false;

    std::vector<const CatData *> sharedParents;
    std::vector<CommonAncestor> commonAncestors;
};

const CatData *FindCat(
    const SaveData &save,
    uint64_t catId);

const CatData *GetParentA(
    const SaveData &save,
    const CatData &cat);

const CatData *GetParentB(
    const SaveData &save,
    const CatData &cat);

std::vector<const CatData *> GetChildren(
    const SaveData &save,
    const CatData &cat);

std::vector<const CatData *> GetParents(
    const SaveData &save,
    const CatData &cat);

std::vector<CatAncestor> GetAncestors(
    const SaveData &save,
    const CatData &cat,
    size_t maxDepth);

std::vector<CommonAncestor> GetCommonAncestors(
    const SaveData &save,
    const CatData &catA,
    const CatData &catB,
    size_t maxDepth);

CatPopulationStats GetPopulationStats(const SaveData &save);

CatInspection InspectCat(
    const SaveData &save,
    const CatData &cat);

CatRelationship AnalyzeRelationship(
    const SaveData &save,
    const CatData &catA,
    const CatData &catB,
    size_t maxDepth);

CatRelationshipType ClassifyRelationship(
    const CatRelationship &relationship);