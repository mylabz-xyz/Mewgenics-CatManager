#include "CatManagerView.h"

#include "../Logger.h"
#include "../Save/Analysis/CatInspector.h"
#include "mew_ui_api.h"

#include <cstdio>
#include <string>

namespace
{
    const CatData *GetSelectedCatForView(
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
}

std::string BuildCatDetails(
    const CatData &cat)
{
    char buffer[4096] = {};

    std::snprintf(
        buffer,
        sizeof(buffer),
        "CAT: %s\n"
        "SEX: %s\n"
        "LEVEL: %d\n"
        "AGE: %d\n"
        "STATUS: %s\n"
        "ROOM: %s\n"
        "\n"
        "PARENT A: %s\n"
        "PARENT B: %s\n"
        "CHILDREN: %zu\n"
        "COI: %.2f%%",
        cat.name.c_str(),
        cat.sex.c_str(),
        cat.level,
        cat.age,
        cat.dead ? "Dead" : "Living",
        cat.room.c_str(),
        cat.parentAId != 0U
            ? "Known"
            : "?",
        cat.parentBId != 0U
            ? "Known"
            : "?",
        cat.children.size(),
        cat.coi * 100.0);

    return buffer;
}

void UpdateCatManagerText(
    const CatManagerState &state,
    bool &textReady)
{
    if (textReady)
        return;

    if (!state.saveData)
        return;

    const CatData *cat =
        GetSelectedCatForView(state);

    if (!cat)
        return;

    const std::string text =
        BuildCatDetails(*cat);

    if (!MewUI_SetTextFromLocalizationKeyValue(
            "House",
            "test_text",
            "CATMANAGER_RAW",
            text.c_str()))
    {
        return;
    }

    textReady = true;

    Log(
        "[UI] CatManager text ready: %s",
        text.c_str());
}

void RenderSelectedCat(
    const CatManagerState &state,
    bool &textReady)
{
    textReady = false;

    UpdateCatManagerText(
        state,
        textReady);
}