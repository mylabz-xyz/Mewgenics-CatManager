#include "FamilyBuilder.h"
#include "../../Logger.h"

void BuildFamilyTree(SaveData &save)
{
    Log("[CatManager] Building family tree");

    size_t linked = 0;

    for (const auto &[pedId, node] : save.pedigree)
    {
        auto childSql = save.sqlToCat.find(pedId);
        if (childSql == save.sqlToCat.end())
            continue;

        uint64_t childId = childSql->second;
        auto childIt = save.cats.find(childId);
        if (childIt == save.cats.end())
            continue;

        CatData &child = childIt->second;
        child.coi = node.coi;

        if (node.parentA > 0)
        {
            auto parentSql = save.sqlToCat.find(node.parentA);
            if (parentSql != save.sqlToCat.end())
            {
                auto parentCat = save.cats.find(parentSql->second);
                if (parentCat != save.cats.end())
                {
                    child.parentAId = parentCat->second.id;
                    parentCat->second.children.push_back(child.id);
                    linked++;
                }
            }
        }

        if (node.parentB > 0)
        {
            auto parentSql = save.sqlToCat.find(node.parentB);
            if (parentSql != save.sqlToCat.end())
            {
                auto parentCat = save.cats.find(parentSql->second);
                if (parentCat != save.cats.end())
                {
                    child.parentBId = parentCat->second.id;
                    parentCat->second.children.push_back(child.id);
                    linked++;
                }
            }
        }
    }

    size_t withChildren = 0;
    for (auto &[id, cat] : save.cats)
    {
        if (!cat.children.empty())
            withChildren++;
    }

    Log("[CatManager builder] Cats with children=%zu", withChildren);
    Log("[CatManager builder] Family links=%zu", linked);
}