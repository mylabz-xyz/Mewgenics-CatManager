#include "BreedingAnalyzer.h"

#include "CatInspector.h"

#include <cmath>

BreedingAnalysis AnalyzeBreeding(
    const SaveData &save,
    const CatData &catA,
    const CatData &catB,
    size_t maxDepth)
{
    BreedingAnalysis result;
    result.catA = &catA;
    result.catB = &catB;
    result.valid = true;

    const std::vector<CommonAncestor> commonAncestors =
        GetCommonAncestors(save, catA, catB, maxDepth);

    for (const CommonAncestor &common : commonAncestors)
    {
        if (!common.cat)
            continue;

        const CatInspection ancestorInspection =
            InspectCat(save, *common.cat);

        const double ancestorCoi =
            ancestorInspection.coi;

        const double contribution =
            std::pow(
                0.5,
                static_cast<double>(
                    common.depthA + common.depthB + 1U))
            * (1.0 + ancestorCoi);

        result.commonAncestors.push_back({
            common.cat,
            common.depthA,
            common.depthB,
            ancestorCoi,
            contribution
        });

        result.expectedOffspringCoi += contribution;
    }

    return result;
}