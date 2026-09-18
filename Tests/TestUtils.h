#pragma once

#include "../Native/Save/Model/SaveData.h"

#include <cstdlib>
#include <iostream>
#include <string>

inline void Check(
    bool condition,
    const char *message)
{
    if (condition)
        return;

    std::cerr
        << "[FAIL] "
        << message
        << '\n';

    std::exit(1);
}

inline CatData MakeCat(
    uint64_t id,
    int64_t sqlKey,
    const char *name)
{
    CatData cat;

    cat.id = id;
    cat.sqlKey = sqlKey;
    cat.name = name;

    return cat;
}