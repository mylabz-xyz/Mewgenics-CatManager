#include <windows.h>

#include "Globals.h"
#include "Mod.h"
#include "Save/SaveLocator.h"
#include "Save/Repository/SaveParser.h"
#include "Save/Analysis/FamilyBuilder.h"
#include "UI/CatManagerUI.h"
#include "Logger.h"

static SaveData g_currentSave;

void ShutdownCatManager()
{
    CatManagerUI_Shutdown();
}

BOOL APIENTRY DllMain(HMODULE hModule, DWORD reason, LPVOID)
{
    if (reason == DLL_PROCESS_ATTACH)
    {
        DisableThreadLibraryCalls(hModule);

        if (!MJ_Require("CatManager"))
            return TRUE;

        if (!MJ_Resolve(&g_mj))
            return TRUE;

        Mod_Init();
    }
    else if (reason == DLL_PROCESS_DETACH)
    {
        ShutdownCatManager();
    }

    return TRUE;
}

void InitCatManager()
{
    char dllPath[MAX_PATH] = {};
    HMODULE hMod = nullptr;

    GetModuleHandleExA(
        GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS,
        (LPCSTR)&InitCatManager,
        &hMod);

    GetModuleFileNameA(
        hMod,
        dllPath,
        MAX_PATH);

    std::string msg = "[CatManager] DLL PATH: ";
    msg += dllPath;
    Log(msg.c_str());

    auto path = FindSave();

    if (path.empty())
    {
        Log("[CatManager] No save found");
        return;
    }

    g_currentSave = ParseSave(path.c_str());

    BuildFamilyTree(g_currentSave);

    Log(
        ("[CatManager] Cats: " +
         std::to_string(g_currentSave.cats.size())).c_str());

    CatManagerUI_Init(&g_currentSave);
}