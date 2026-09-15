#pragma once

struct sqlite3;
struct SaveData;
#include <unordered_map>
#include "../Model/PedigreeNode.h"

void LoadPedigree(
    sqlite3 *db,
    SaveData &save);