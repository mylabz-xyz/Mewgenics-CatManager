#include "CatManagerSearch.h"

#include "../Logger.h"
#include "../Save/Analysis/CatInspector.h"

#include <algorithm>
#include <cstdint>

void RebuildCatOrder(
    CatManagerState &state)
{
    state.catOrder.clear();
    state.selectedCatIndex = 0U;

    if (!state.saveData)
        return;

    for (const auto &[id, cat] :
         state.saveData->cats)
    {
        if (cat.dead)
            continue;

        state.catOrder.push_back(id);
    }

    std::sort(
        state.catOrder.begin(),
        state.catOrder.end(),
        [&state](uint64_t a, uint64_t b)
        {
            const CatData &catA =
                state.saveData->cats.at(a);

            const CatData &catB =
                state.saveData->cats.at(b);

            if (catA.name != catB.name)
                return catA.name < catB.name;

            return catA.id < catB.id;
        });

    Log(
        "[CatManager] Living cats: %zu",
        state.catOrder.size());
}

const CatData *GetSelectedCat(
    const CatManagerState &state)
{
    if (!state.saveData)
        return nullptr;

    if (state.selectedCatIndex >=
        state.catOrder.size())
        return nullptr;

    const uint64_t catId =
        state.catOrder[state.selectedCatIndex];

    const auto it =
        state.saveData->cats.find(catId);

    if (it == state.saveData->cats.end())
        return nullptr;

    return &it->second;
}

bool SelectLivingCat(
    CatManagerState &state,
    uint64_t catId)
{
    if (!state.saveData)
        return false;

    const auto catIt =
        state.saveData->cats.find(catId);

    if (catIt == state.saveData->cats.end())
        return false;

    // The main navigation currently contains living cats only.
    // Dead cats can still appear in pedigree data, but are not selectable here yet.
    if (catIt->second.dead)
        return false;

    const auto orderIt =
        std::find(
            state.catOrder.begin(),
            state.catOrder.end(),
            catId);

    if (orderIt == state.catOrder.end())
        return false;

    state.selectedCatIndex =
        static_cast<size_t>(
            std::distance(
                state.catOrder.begin(),
                orderIt));

    return true;
}