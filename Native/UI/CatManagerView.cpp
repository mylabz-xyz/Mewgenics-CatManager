#include "CatManagerView.h"

#include "../Logger.h"
#include "../Save/Analysis/CatInspector.h"
#include "mew_ui_api.h"
#include "../Save/Analysis/BreedingAnalyzer.h"

#include <cstdio>
#include <string>
#include <algorithm>
#include <cstring>

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
    const SaveData &save,
    const CatData &cat)
{
    const CatInspection inspection =
        InspectCat(
            save,
            cat);

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
        inspection.parentA
            ? inspection.parentA->name.c_str()
            : "?",
        inspection.parentB
            ? inspection.parentB->name.c_str()
            : "?",
        inspection.children.size(),
        inspection.coi * 100.0);

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
        BuildCatDetails(
            *state.saveData,
            *cat);

    const char *value = "HELLO";

    if (!MewUI_SetTextFromLocalizationKeyValue(
            "House",
            "test_text",
            "TEST_BUTTON_TEXT_VALUE",
            value))
    {
        Log("[UI] Failed to set test_text");
        return;
    }

    textReady = true;
    Log("[UI] test_text updated");
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

const char *GetBreedingRisk(double expectedCoi)
{
    const double coiPercent =
        expectedCoi * 100.0;

    if (coiPercent < 12.5)
        return "LOW";

    if (coiPercent < 25.0)
        return "MEDIUM";

    return "HIGH";
}
void UpdateBreedingSelectionText(
    const CatManagerState &state,
    bool &textReady)
{
    if (textReady)
        return;

    if (!state.saveData)
        return;

    const CatData *catA = nullptr;
    const CatData *catB = nullptr;

    if (state.breedingCatAId != 0U)
    {
        const auto it =
            state.saveData->cats.find(
                state.breedingCatAId);

        if (it != state.saveData->cats.end())
            catA = &it->second;
    }

    if (state.breedingCatBId != 0U)
    {
        const auto it =
            state.saveData->cats.find(
                state.breedingCatBId);

        if (it != state.saveData->cats.end())
            catB = &it->second;
    }

    const char *nameA =
        catA
            ? catA->name.c_str()
            : "SELECT CAT";

    const char *nameB =
        catB
            ? catB->name.c_str()
            : "SELECT CAT";

    /*
     * Update the two dedicated TextFields.
     */
    if (!MewUI_SetTextFromLocalizationKeyValue(
            "House",
            "breeding_cat_a",
            "CATMANAGER_NAV_VAL",
            nameA))
    {
        return;
    }

    if (!MewUI_SetTextFromLocalizationKeyValue(
            "House",
            "breeding_cat_b",
            "CATMANAGER_NAV_VAL",
            nameB))
    {
        return;
    }

    /*
     * Update the COI information
     * for both selected cats.
     */
    char infoA[256] = {};
    char infoB[256] = {};

    if (catA)
    {
        std::snprintf(
            infoA,
            sizeof(infoA),
            "COI %.2f%%",
            catA->coi * 100.0);
    }
    else
    {
        std::snprintf(
            infoA,
            sizeof(infoA),
            "COI 0.00%%");
    }

    if (catB)
    {
        std::snprintf(
            infoB,
            sizeof(infoB),
            "COI %.2f%%",
            catB->coi * 100.0);
    }
    else
    {
        std::snprintf(
            infoB,
            sizeof(infoB),
            "COI 0.00%%");
    }

    if (!MewUI_SetTextFromLocalizationKeyValue(
            "House",
            "breeding_cat_a_info",
            "CATMANAGER_RAW",
            infoA))
    {
        return;
    }

    if (!MewUI_SetTextFromLocalizationKeyValue(
            "House",
            "breeding_cat_b_info",
            "CATMANAGER_RAW",
            infoB))
    {
        return;
    }

    /*
     * Calculate breeding analysis once.
     */
    BreedingAnalysis analysis{};

    if (catA && catB)
    {
        analysis =
            AnalyzeBreeding(
                *state.saveData,
                *catA,
                *catB,
                10U);
    }

    /*
     * Update breeding risk.
     */
    char riskText[64] = {};

    if (!catA || !catB)
    {
        std::snprintf(
            riskText,
            sizeof(riskText),
            "RISK: -");
    }
    else
    {
        std::snprintf(
            riskText,
            sizeof(riskText),
            "RISK: %s",
            GetBreedingRisk(
                analysis.expectedOffspringCoi));
    }

    if (!MewUI_SetTextFromLocalizationKeyValue(
            "House",
            "breeding_risk",
            "CATMANAGER_RAW",
            riskText))
    {
        return;
    }

    /*
     * Update common ancestors.
     */
    char ancestorsText[512] = {};

    if (!catA || !catB)
    {
        std::snprintf(
            ancestorsText,
            sizeof(ancestorsText),
            "COMMON ANCESTORS: -");
    }
    else if (analysis.commonAncestors.empty())
    {
        std::snprintf(
            ancestorsText,
            sizeof(ancestorsText),
            "COMMON ANCESTORS: NONE");
    }
    else
    {
        std::snprintf(
            ancestorsText,
            sizeof(ancestorsText),
            "COMMON ANCESTORS:");

        size_t offset =
            std::strlen(ancestorsText);

        const size_t maxDisplayed = 4U;

        const size_t count =
            analysis.commonAncestors.size() < maxDisplayed
                ? analysis.commonAncestors.size()
                : maxDisplayed;

        for (size_t i = 0U; i < count; ++i)
        {
            const BreedingCommonAncestor &common =
                analysis.commonAncestors[i];

            if (!common.ancestor)
                continue;

            const int written =
                std::snprintf(
                    ancestorsText + offset,
                    sizeof(ancestorsText) - offset,
                    "\n%s",
                    common.ancestor->name.c_str());

            if (written <= 0)
                break;

            offset +=
                static_cast<size_t>(written);

            if (offset >= sizeof(ancestorsText))
            {
                offset =
                    sizeof(ancestorsText) - 1U;
                break;
            }
        }

        if (analysis.commonAncestors.size() > maxDisplayed &&
            offset < sizeof(ancestorsText))
        {
            std::snprintf(
                ancestorsText + offset,
                sizeof(ancestorsText) - offset,
                "\n...");
        }
    }

    if (!MewUI_SetTextFromLocalizationKeyValue(
            "House",
            "breeding_common_ancestors",
            "CATMANAGER_RAW",
            ancestorsText))
    {
        return;
    }

    textReady = true;

    Log(
        "[UI] Breeding selection text ready: A='%s' B='%s'",
        nameA,
        nameB);
}