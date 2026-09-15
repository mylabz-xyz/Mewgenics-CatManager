#include "SaveLocator.h"

#include <filesystem>
#include <windows.h>

std::string FindSave()
{
    char *appdata = nullptr;
    size_t len = 0;

    _dupenv_s(
        &appdata,
        &len,
        "APPDATA");

    if (!appdata)
        return "";

    std::filesystem::path root =
        std::filesystem::path(appdata) / "Glaiel Games" / "Mewgenics";

    free(appdata);

    if (!std::filesystem::exists(root))
        return "";

    for (auto &entry : std::filesystem::recursive_directory_iterator(root))
    {
        if (!entry.is_regular_file())
            continue;

        if (entry.path().extension() == ".sav")
        {
            return entry.path().string();
        }
    }

    return "";
}