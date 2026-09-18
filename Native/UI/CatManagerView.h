#pragma once

#include "CatManagerState.h"

#include <string>

std::string BuildCatDetails(
    const SaveData &save,
    const CatData &cat);

void UpdateCatManagerText(
    const CatManagerState &state,
    bool &textReady);

void RenderSelectedCat(
    const CatManagerState &state,
    bool &textReady);

void UpdateBreedingSelectionText(
    const CatManagerState &state,
    bool &textReady);