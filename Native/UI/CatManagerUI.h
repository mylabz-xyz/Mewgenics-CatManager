#pragma once

#include "../Save/Model/SaveData.h"

void CatManagerUI_Init(const SaveData* saveData);
void CatManagerUI_UpdateSaveData(const SaveData* saveData);
void CatManagerUI_Shutdown();