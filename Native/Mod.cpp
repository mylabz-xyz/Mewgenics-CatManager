#include "Mod.h"
#include "Hooks.h"
#include "Logger.h"
#include "CatManager.h"

bool Mod_Init()
{
    Log("Cat Manager initialized");
    // Hooks_Initialize();
    InitCatManager();

    return true;
}