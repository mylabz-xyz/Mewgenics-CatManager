#include "FamilyBuilder.h"

#include "../../Logger.h"

namespace
{
    // Convert a pedigree SQL key into the corresponding CatData.
    // Pedigree nodes use SQL keys, while CatManager uses cat IDs.
    CatData *FindCatBySqlKey(
        SaveData &save,
        int64_t sqlKey)
    {
        if (sqlKey <= 0)
            return nullptr;

        const auto sqlIt = save.sqlToCat.find(sqlKey);

        if (sqlIt == save.sqlToCat.end())
            return nullptr;

        const auto catIt = save.cats.find(sqlIt->second);

        if (catIt == save.cats.end())
            return nullptr;

        return &catIt->second;
    }
}

// Build parent/child relationships from the parsed pedigree.
// Parent A/B are kept as stored; they are not interpreted as father/mother.
void BuildFamilyTree(SaveData &save)
{
    Log("[CatManager] Building family tree");

    size_t linked = 0;

    for (const auto &[pedId, node] : save.pedigree)
    {
        CatData *child = FindCatBySqlKey(save, pedId);

        if (!child)
            continue;

        child->coi = node.coi;

        if (node.parentA > 0)
        {
            CatData *parentA =
                FindCatBySqlKey(save, node.parentA);

            if (parentA)
            {
                child->parentAId = parentA->id;
                parentA->children.push_back(child->id);
                ++linked;
            }
        }

        if (node.parentB > 0)
        {
            CatData *parentB =
                FindCatBySqlKey(save, node.parentB);

            if (parentB)
            {
                child->parentBId = parentB->id;
                parentB->children.push_back(child->id);
                ++linked;
            }
        }
    }

    size_t withChildren = 0;

    for (auto &[id, cat] : save.cats)
    {
        (void)id;

        if (!cat.children.empty())
            ++withChildren;
    }

    Log("[CatManager builder] Cats with children=%zu", withChildren);
    Log("[CatManager builder] Family links=%zu", linked);
}