#pragma once

#include "CatManagerState.h"

#include <string>

std::string BuildCatDetails(
    const CatData &cat);

void UpdateCatManagerText(
    const CatManagerState &state,
    bool &textReady);

void RenderSelectedCat(
    const CatManagerState &state,
    bool &textReady);