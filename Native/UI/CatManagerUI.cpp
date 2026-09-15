#include "CatManagerUI.h"

#include "mew_ui_api.h"
#include "../Logger.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>

// ============================================================
// Configuration
// ============================================================

namespace
{
    constexpr const char *MOD_NAME = "CatManager";
    constexpr const char *SCENE_NAME = "House";

    // Nodes du SWF de test officiel.
    constexpr const char *TEXT_NODE_NAME = "test_text";
    constexpr const char *BUTTON_NODE_NAME = "test_button";

    // Localization
    constexpr const char *BUTTON_ROLE_NAME = "CatManager_Button";
    constexpr const char *BUTTON_LABEL_KEY = "TEST_BUTTON_TEXT";
    constexpr const char *BUTTON_CLICK_LABEL_KEY = "TEST_BUTTON_TEXT_VALUE";

    constexpr int MEW_UI_HOOK_PRIORITY = 30;

    // Le callback MewUI n'a pas besoin d'être interrogé à chaque frame.
    constexpr uint32_t UI_BOOTSTRAP_INTERVAL_MS = 500U;
    constexpr uint32_t UI_TICK_INTERVAL_MS = 250U;
}

// ============================================================
// UI state
// ============================================================

namespace
{
    const SaveData *g_saveData = nullptr;

    MewUISceneBinding g_houseScene{};

    void *g_catManagerButton = nullptr;

    bool g_sceneReady = false;
    bool g_buttonReady = false;
    bool g_textReady = false;

    uint32_t g_buttonClicks = 0U;

    std::vector<uint64_t> g_catOrder;
    size_t g_selectedCatIndex = 0U;
}

// ============================================================
// Forward declarations
// ============================================================

namespace
{
    void RebuildCatOrder();
    void ResetSceneState();

    const CatData *GetSelectedCat();

    std::string GetParentName(int64_t sqlKey);
    std::string BuildCatDetails(const CatData &cat);

    void TrySetupButton();
    void TryUpdateText();

    void UpdateButtonLabel();
    void RenderSelectedCat();

    void __cdecl CatManagerUITick(void *userData);

    void __cdecl SceneRefreshCallback(
        MewUISceneBinding *binding,
        MewUISceneRefreshResult result,
        void *oldSceneManager,
        void *newSceneManager,
        void *userData);

    void __cdecl CatManagerButtonCallback(
        void *button,
        MewButtonEvent eventType,
        MewButtonState oldState,
        MewButtonState newState,
        void *userData);
}

// ============================================================
// Cat list
// ============================================================

namespace
{
    void RebuildCatOrder()
    {
        g_catOrder.clear();
        g_selectedCatIndex = 0U;

        if (!g_saveData)
            return;

        g_catOrder.reserve(g_saveData->cats.size());

        for (const auto &[id, cat] : g_saveData->cats)
        {
            if (!cat.dead)
                g_catOrder.push_back(id);
        }

        std::sort(
            g_catOrder.begin(),
            g_catOrder.end(),
            [](uint64_t a, uint64_t b)
            {
                const auto &catA = g_saveData->cats.at(a);
                const auto &catB = g_saveData->cats.at(b);

                if (catA.name != catB.name)
                    return catA.name < catB.name;

                return a < b;
            });

        Log(
            "[UI] Cat order rebuilt: %zu living cats",
            g_catOrder.size());
    }

    const CatData *GetSelectedCat()
    {
        if (!g_saveData || g_catOrder.empty())
            return nullptr;

        if (g_selectedCatIndex >= g_catOrder.size())
            g_selectedCatIndex = 0U;

        const uint64_t catId = g_catOrder[g_selectedCatIndex];

        auto it = g_saveData->cats.find(catId);
        if (it == g_saveData->cats.end())
            return nullptr;

        return &it->second;
    }
}

// ============================================================
// Pedigree helpers
// ============================================================

namespace
{
    std::string GetParentName(int64_t sqlKey)
    {
        if (!g_saveData || sqlKey <= 0)
            return "None";

        auto sqlIt = g_saveData->sqlToCat.find(sqlKey);

        if (sqlIt == g_saveData->sqlToCat.end())
            return "Historical #" + std::to_string(sqlKey);

        auto catIt = g_saveData->cats.find(sqlIt->second);

        if (catIt == g_saveData->cats.end())
            return "Historical #" + std::to_string(sqlKey);

        return catIt->second.name;
    }

    std::string BuildCatDetails(const CatData &cat)
    {
        size_t living = 0U;
        size_t deceased = 0U;

        for (const auto &[id, item] : g_saveData->cats)
        {
            (void)id;

            if (item.dead)
                ++deceased;
            else
                ++living;
        }

        std::string parentA = "None";
        std::string parentB = "None";
        double coi = 0.0;

        auto pedIt = g_saveData->pedigree.find(cat.sqlKey);

        if (pedIt != g_saveData->pedigree.end())
        {
            parentA = GetParentName(pedIt->second.parentA);
            parentB = GetParentName(pedIt->second.parentB);
            coi = pedIt->second.coi;
        }

        char text[768];

        std::snprintf(
            text,
            sizeof(text),
            "CatManager | Cats: %zu | Living: %zu | Dead: %zu\n"
            "%s | %s\n"
            "Parents: %s / %s\n"
            "COI: %.6f",
            g_saveData->cats.size(),
            living,
            deceased,
            cat.name.c_str(),
            cat.sex.c_str(),
            parentA.c_str(),
            parentB.c_str(),
            coi);

        text[sizeof(text) - 1U] = '\0';

        return text;
    }
}

// ============================================================
// Rendering
// ============================================================

namespace
{
    void UpdateButtonLabel()
    {
        if (!g_buttonReady || !g_catManagerButton)
            return;

        const CatData *cat = GetSelectedCat();

        if (!cat)
            return;

        if (!MewUI_SetButtonLabelFromLocalizationKeyValue(
                g_catManagerButton,
                BUTTON_CLICK_LABEL_KEY,
                cat->name.c_str()))
        {
            Log("[UI] Failed to update CatManager button label");
            return;
        }
    }

    void TryUpdateText()
    {
        // Une fois le node trouvé, inutile de le rechercher en permanence.
        if (g_textReady)
            return;

        if (!g_saveData)
            return;

        const CatData *cat = GetSelectedCat();

        if (!cat)
            return;

        const std::string text = BuildCatDetails(*cat);

        if (!MewUI_SetTextFromLocalizationKeyValue(
                SCENE_NAME,
                TEXT_NODE_NAME,
                "CATMANAGER_RAW",
                text.c_str()))
        {
            return;
        }

        g_textReady = true;

        Log(
            "[UI] CatManager text ready: %s",
            text.c_str());
    }

    void RenderSelectedCat()
    {
        const CatData *cat = GetSelectedCat();

        if (!cat)
            return;

        // Le texte doit être réécrit après un changement de sélection.
        g_textReady = false;

        UpdateButtonLabel();
        TryUpdateText();
    }
}

// ============================================================
// Button
// ============================================================

namespace
{
    void TrySetupButton()
    {
        if (g_buttonReady)
            return;

        void *button = nullptr;
        int created = 0;

        button = MewUI_SetupButtonFromLocalizationKey(
            SCENE_NAME,
            BUTTON_NODE_NAME,
            BUTTON_ROLE_NAME,
            BUTTON_LABEL_KEY,
            CatManagerButtonCallback,
            nullptr,
            &g_catManagerButton,
            &created);

        if (!button)
        {
            g_catManagerButton = nullptr;
            return;
        }

        g_catManagerButton = button;
        g_buttonReady = true;

        if (created)
        {
            Log(
                "[UI] CatManager button created: button=%p",
                button);
        }
        else
        {
            Log(
                "[UI] CatManager button found: button=%p",
                button);
        }

        UpdateButtonLabel();
        TryUpdateText();
    }

    void __cdecl CatManagerButtonCallback(
        void *button,
        MewButtonEvent eventType,
        MewButtonState oldState,
        MewButtonState newState,
        void *userData)
    {
        (void)button;
        (void)userData;

        Log(
            "[UI] CatManager button event: %s (%s -> %s)",
            MewUI_GetButtonEventName(eventType),
            MewUI_GetButtonStateName(oldState),
            MewUI_GetButtonStateName(newState));

        if (eventType != MEW_BUTTON_EVENT_CLICK)
            return;

        ++g_buttonClicks;

        if (!g_catOrder.empty())
        {
            ++g_selectedCatIndex;

            if (g_selectedCatIndex >= g_catOrder.size())
                g_selectedCatIndex = 0U;
        }

        RenderSelectedCat();

        Log(
            "[UI] CatManager selected cat %zu/%zu: %s",
            g_catOrder.empty() ? 0U : g_selectedCatIndex + 1U,
            g_catOrder.size(),
            GetSelectedCat() ? GetSelectedCat()->name.c_str() : "<none>");
    }
}

// ============================================================
// Scene lifecycle
// ============================================================

namespace
{
    void ResetSceneState()
    {
        g_sceneReady = false;
        g_buttonReady = false;
        g_textReady = false;
        g_catManagerButton = nullptr;
    }

    void __cdecl SceneRefreshCallback(
        MewUISceneBinding *binding,
        MewUISceneRefreshResult result,
        void *oldSceneManager,
        void *newSceneManager,
        void *userData)
    {
        (void)binding;
        (void)userData;

        Log(
            "[UI] Scene refresh: %s old=%p new=%p",
            MewUI_GetSceneRefreshResultName(result),
            oldSceneManager,
            newSceneManager);

        if (result == MEW_UI_SCENE_REFRESH_LOADED)
        {
            g_sceneReady = true;
            g_buttonReady = false;
            g_textReady = false;
            g_catManagerButton = nullptr;

            Log("[UI] House scene loaded");
            return;
        }

        if (result == MEW_UI_SCENE_REFRESH_CHANGED)
        {
            g_sceneReady = true;
            g_buttonReady = false;
            g_textReady = false;
            g_catManagerButton = nullptr;

            Log("[UI] House scene changed");
            return;
        }

        if (result == MEW_UI_SCENE_REFRESH_UNLOADED)
        {
            ResetSceneState();
            Log("[UI] House scene unloaded");
        }

        // Go to game home
        if (result == MEW_UI_SCENE_REFRESH_CHANGED ||
            result == MEW_UI_SCENE_REFRESH_UNLOADED)
        {
            g_catManagerButton = nullptr;
            g_buttonReady = false;
            g_textReady = false;
            g_sceneReady = false;
        }
    }

    void __cdecl CatManagerUITick(void *userData)
    {
        (void)userData;

        MewUI_RefreshSceneBinding(&g_houseScene);

        void *sceneManager =
            MewUI_GetSceneBindingScene(&g_houseScene);

        if (!sceneManager)
            return;

        if (!g_sceneReady)
            return;

        // Chaque opération n'est réellement tentée que tant
        // qu'elle n'est pas encore prête.
        TrySetupButton();

        if (g_buttonReady && !g_textReady)
            TryUpdateText();
    }
}

// ============================================================
// Public API
// ============================================================

void CatManagerUI_Init(const SaveData *saveData)
{
    Log("[UI] CatManagerUI_Init");

    g_saveData = saveData;

    g_buttonClicks = 0U;
    g_selectedCatIndex = 0U;

    ResetSceneState();
    RebuildCatOrder();

    MewUI_InitSceneBinding(
        &g_houseScene,
        SCENE_NAME,
        SceneRefreshCallback,
        nullptr);

    MewUI_SetDebugLogsEnabled(true);

    const int result = MewUI_Start(
        MOD_NAME,
        MEW_UI_HOOK_PRIORITY,
        UI_BOOTSTRAP_INTERVAL_MS,
        UI_TICK_INTERVAL_MS,
        CatManagerUITick,
        nullptr);

    Log(
        "[UI] MewUI_Start result=%d",
        result);
}

void CatManagerUI_UpdateSaveData(const SaveData *saveData)
{
    g_saveData = saveData;

    g_selectedCatIndex = 0U;

    RebuildCatOrder();

    // Si l'UI existe déjà, on rafraîchit immédiatement.
    if (g_buttonReady)
    {
        g_textReady = false;
        UpdateButtonLabel();
        TryUpdateText();
    }
}

void CatManagerUI_Shutdown()
{
    Log("[UI] CatManagerUI_Shutdown");

    ResetSceneState();

    MewUI_ClearSceneBinding(&g_houseScene);
    MewUI_Stop();

    g_saveData = nullptr;

    g_catOrder.clear();
    g_selectedCatIndex = 0U;
    g_buttonClicks = 0U;
}