#include "Hooks.h"
#include "Logger.h"
#include "Globals.h"


typedef unsigned int (__cdecl *SDL_GetTicks_t)();

SDL_GetTicks_t original_GetTicks = nullptr;

unsigned int Hook_GetTicks()
{
    static int count = 0;

    if (++count == 1)
        Log("SDL_GetTicks hook OK");

    return original_GetTicks();
}

bool Hooks_Initialize()
{
    g_mj.InstallHook(
        0xB9CC90,
        12,
        (void*)Hook_GetTicks,
        (void**)&original_GetTicks,
        100,
        "CatManager"
    );

    return true;
}