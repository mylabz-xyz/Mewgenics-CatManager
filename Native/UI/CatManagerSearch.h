#pragma once

#include "CatManagerState.h"

bool SelectLivingCat(
    CatManagerState &state,
    uint64_t catId);

void RebuildCatOrder(
    CatManagerState &state);

const CatData *GetSelectedCat(
    const CatManagerState &state);