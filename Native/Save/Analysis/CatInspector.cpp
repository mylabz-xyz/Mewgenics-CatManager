#include "CatInspector.h"
#include "../../Logger.h"

#include <unordered_map>
#include <algorithm>
#include <cctype>
#include <string>

namespace
{
    // Resolve a cat ID through the parsed save data.
    // Keeping ID lookup here avoids duplicating map access in the inspectors.
    const CatData *FindCatById(
        const SaveData &save,
        uint64_t catId)
    {
        if (catId == 0)
            return nullptr;

        const auto it = save.cats.find(catId);
        if (it == save.cats.end())
            return nullptr;

        return &it->second;
    }
}

namespace
{
    std::string ToLower(const std::string &value)
    {
        std::string result;
        result.reserve(value.size());

        for (const unsigned char c : value)
            result.push_back(
                static_cast<char>(std::tolower(c)));

        return result;
    }
}

std::vector<CatSearchResult> SearchCats(
    const SaveData &save,
    const std::string &query)
{
    std::vector<CatSearchResult> result;

    const std::string normalizedQuery =
        ToLower(query);

    if (normalizedQuery.empty())
        return result;

    for (const auto &[id, cat] : save.cats)
    {
        (void)id;

        if (cat.dead)
            continue;

        const std::string normalizedName =
            ToLower(cat.name);

        const size_t position =
            normalizedName.find(normalizedQuery);

        if (position == std::string::npos)
            continue;

        result.push_back({&cat,
                          normalizedName == normalizedQuery});
    }

    std::sort(
        result.begin(),
        result.end(),
        [](const CatSearchResult &a,
           const CatSearchResult &b)
        {
            if (a.exactMatch != b.exactMatch)
                return a.exactMatch > b.exactMatch;

            if (a.cat->name != b.cat->name)
                return a.cat->name < b.cat->name;

            return a.cat->id < b.cat->id;
        });

    return result;
}

const CatData *FindCat(
    const SaveData &save,
    uint64_t catId)
{
    return FindCatById(save, catId);
}

const CatData *GetParentA(
    const SaveData &save,
    const CatData &cat)
{
    // Parent order follows the pedigree data from the save.
    // Do not infer father/mother from this field.
    return FindCatById(save, cat.parentAId);
}

const CatData *GetParentB(
    const SaveData &save,
    const CatData &cat)
{
    return FindCatById(save, cat.parentBId);
}

std::vector<const CatData *> GetChildren(
    const SaveData &save,
    const CatData &cat)
{
    // Child IDs come from the parsed pedigree.
    // Unresolved IDs are ignored rather than exposing invalid references.
    std::vector<const CatData *> result;
    result.reserve(cat.children.size());

    for (const uint64_t childId : cat.children)
    {
        const CatData *child = FindCatById(save, childId);

        if (child)
            result.push_back(child);
    }

    return result;
}

CatPopulationStats GetPopulationStats(const SaveData &save)
{
    CatPopulationStats stats;
    stats.total = save.cats.size();

    for (const auto &[id, cat] : save.cats)
    {
        (void)id;

        if (cat.dead)
            ++stats.deceased;
        else
            ++stats.living;
    }

    return stats;
}

CatInspection InspectCat(
    const SaveData &save,
    const CatData &cat)
{
    // Centralize all data needed by the cat details view.
    // This keeps UI formatting separate from save-data inspection.
    CatInspection inspection;

    inspection.population = GetPopulationStats(save);
    inspection.parentA = GetParentA(save, cat);
    inspection.parentB = GetParentB(save, cat);
    inspection.children = GetChildren(save, cat);
    inspection.coi = cat.coi;

    return inspection;
}

std::vector<const CatData *> GetParents(
    const SaveData &save,
    const CatData &cat)
{
    std::vector<const CatData *> result;
    result.reserve(2);

    if (const CatData *parentA = GetParentA(save, cat))
        result.push_back(parentA);

    if (const CatData *parentB = GetParentB(save, cat))
        result.push_back(parentB);

    return result;
}

// Walk the pedigree level by level, starting from the selected cat's parents.
// The depth limits how many generations are traversed.
// Parent A/B remain in save order; no father/mother interpretation is made.
std::vector<CatAncestor> GetAncestors(
    const SaveData &save,
    const CatData &cat,
    size_t maxDepth)
{
    std::vector<CatAncestor> result;

    if (maxDepth == 0)
        return result;

    std::vector<const CatData *> currentLevel =
        GetParents(save, cat);

    for (size_t depth = 1; depth <= maxDepth; ++depth)
    {
        if (currentLevel.empty())
            break;

        std::vector<const CatData *> nextLevel;

        for (const CatData *ancestor : currentLevel)
        {
            if (!ancestor)
                continue;

            result.push_back({ancestor,
                              depth});

            const std::vector<const CatData *> parents =
                GetParents(save, *ancestor);

            nextLevel.insert(
                nextLevel.end(),
                parents.begin(),
                parents.end());
        }

        currentLevel = std::move(nextLevel);
    }

    return result;
}
std::vector<CommonAncestor> GetCommonAncestors(
    const SaveData &save,
    const CatData &catA,
    const CatData &catB,
    size_t maxDepth)
{
    std::vector<CommonAncestor> result;

    if (maxDepth == 0)
        return result;

    const std::vector<CatAncestor> ancestorsA =
        GetAncestors(save, catA, maxDepth);

    const std::vector<CatAncestor> ancestorsB =
        GetAncestors(save, catB, maxDepth);

    std::unordered_map<uint64_t, CommonAncestor> commonById;

    for (const CatAncestor &ancestorA : ancestorsA)
    {
        if (!ancestorA.cat)
            continue;

        for (const CatAncestor &ancestorB : ancestorsB)
        {
            if (!ancestorB.cat)
                continue;

            if (ancestorA.cat->id != ancestorB.cat->id)
                continue;

            const uint64_t id = ancestorA.cat->id;

            auto it = commonById.find(id);

            if (it == commonById.end())
            {
                commonById.emplace(
                    id,
                    CommonAncestor{
                        ancestorA.cat,
                        ancestorA.depth,
                        ancestorB.depth});
                continue;
            }

            CommonAncestor &existing = it->second;

            existing.depthA =
                std::min(existing.depthA, ancestorA.depth);

            existing.depthB =
                std::min(existing.depthB, ancestorB.depth);
        }
    }

    result.reserve(commonById.size());

    for (const auto &[id, common] : commonById)
    {
        (void)id;
        result.push_back(common);
    }

    return result;
}

CatRelationship AnalyzeRelationship(
    const SaveData &save,
    const CatData &catA,
    const CatData &catB,
    size_t maxDepth)
{
    CatRelationship relationship;

    relationship.isParent =
        catA.parentAId == catB.id ||
        catA.parentBId == catB.id;

    relationship.isChild =
        catB.parentAId == catA.id ||
        catB.parentBId == catA.id;

    const std::vector<const CatData *> parentsA =
        GetParents(save, catA);

    const std::vector<const CatData *> parentsB =
        GetParents(save, catB);

    for (const CatData *parentA : parentsA)
    {
        if (!parentA)
            continue;

        for (const CatData *parentB : parentsB)
        {
            if (!parentB)
                continue;

            if (parentA->id != parentB->id)
                continue;

            relationship.sharedParents.push_back(parentA);
        }
    }

    relationship.commonAncestors =
        GetCommonAncestors(
            save,
            catA,
            catB,
            maxDepth);

    relationship.type =
        ClassifyRelationship(relationship);

    return relationship;
}

CatRelationshipType ClassifyRelationship(
    const CatRelationship &relationship)
{
    if (relationship.isParent)
        return CatRelationshipType::Parent;

    if (relationship.isChild)
        return CatRelationshipType::Child;

    if (relationship.sharedParents.size() >= 2)
        return CatRelationshipType::FullSibling;

    if (relationship.sharedParents.size() == 1)
        return CatRelationshipType::HalfSibling;

    if (!relationship.commonAncestors.empty())
        return CatRelationshipType::Other;

    return CatRelationshipType::Unrelated;
}