#include "Logger.h"
#include "mewjector.h"
#include "Globals.h"

#include <cstdarg>
#include <cstdio>

void Log(const char* fmt, ...)
{
    char buffer[1024];

    va_list args;
    va_start(args, fmt);

    vsnprintf(buffer, sizeof(buffer), fmt, args);

    va_end(args);

    g_mj.Log("CatManager", "%s", buffer);
}