#include "CatManagerUI.h"

#include "mew_ui_api.h"
#include "../Logger.h"
#include "../Save/Analysis/CatInspector.h"

#include <algorithm>
#include <cstdint>
#include <cstdio>
#include <string>
#include <vector>
#include <deque>

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

    // Navigation
    constexpr const char *NAV_VALUE_NODE_NAME = "test_nav_value";
    constexpr const char *NAV_LEFT_NODE_NAME = "test_nav_left";
    constexpr const char *NAV_RIGHT_NODE_NAME = "test_nav_right";

    constexpr const char *NAV_VALUE_TEXT_KEY = "TEST_NAV_VALUE_TEXT";

    constexpr const char *NAV_LEFT_ROLE_NAME = "CatManager_Nav_Left";
    constexpr const char *NAV_RIGHT_ROLE_NAME = "CatManager_Nav_Right";
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

    // nav
    MewUINavigationBinding g_navigation{};

    bool g_navigationReady = false;

    std::deque<std::string> g_navigationValues;
    std::vector<const char *> g_navigationValuePointers;
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

void __cdecl CatManagerNavigationChangedCallback(
    MewUINavigationBinding *binding,
    uint32_t index,
    const char *value,
    void *userData);

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

        g_navigationValues.clear();
        g_navigationValuePointers.clear();

        g_navigationValues.resize(0);
        g_navigationValuePointers.resize(0);

        g_navigationValues.clear();

        for (uint64_t id : g_catOrder)
        {
            const auto it = g_saveData->cats.find(id);

            if (it == g_saveData->cats.end())
                continue;

            g_navigationValues.push_back(it->second.name);
        }

        g_navigationValuePointers.reserve(g_navigationValues.size());

        for (const std::string &value : g_navigationValues)
        {
            g_navigationValuePointers.push_back(value.c_str());
        }
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

void InitializeNavigationBinding()
{
    MewUI_InitNavigationBinding(
        &g_navigation,
        SCENE_NAME,
        NAV_LEFT_NODE_NAME,
        NAV_RIGHT_NODE_NAME,
        NAV_VALUE_NODE_NAME,
        NAV_LEFT_ROLE_NAME,
        NAV_RIGHT_ROLE_NAME,
        NAV_VALUE_TEXT_KEY,
        g_navigationValuePointers.empty()
            ? nullptr
            : g_navigationValuePointers.data(),
        static_cast<uint32_t>(g_navigationValuePointers.size()),
        static_cast<uint32_t>(g_selectedCatIndex),
        CatManagerNavigationChangedCallback,
        nullptr);
}

void __cdecl CatManagerNavigationChangedCallback(
    MewUINavigationBinding *binding,
    uint32_t index,
    const char *value,
    void *userData)
{
    (void)userData;

    if (!binding || g_catOrder.empty())
        return;

    if (index >= g_catOrder.size())
        return;

    g_selectedCatIndex = static_cast<size_t>(index);

    Log(
        "[UI] Cat navigation: index=%u/%zu value='%s'",
        static_cast<unsigned int>(index + 1U),
        g_catOrder.size(),
        value ? value : "");

    g_textReady = false;
    TryUpdateText();
}

void TrySetupNavigation(void *sceneManager)
{
    if (g_navigationReady || !sceneManager)
        return;

    if (g_navigationValuePointers.empty())
        return;

    int created = 0;

    if (!MewUI_SetupNavigationInScene(
            &g_navigation,
            sceneManager,
            &created))
    {
        return;
    }

    g_navigationReady = true;

    // Synchronise la sélection C++ avec le binding MewUI.
    MewUI_SetNavigationIndex(
        &g_navigation,
        static_cast<uint32_t>(g_selectedCatIndex));

    Log(
        "[UI] Cat navigation ready: left=%p right=%p created=%d",
        g_navigation.left_button,
        g_navigation.right_button,
        created);
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

    // Build the text displayed by the CatManager UI for the selected cat.
    std::string BuildCatDetails(const CatData &cat)
    {
        if (!g_saveData)
            return {};

        // Gather all derived data in one place before formatting the UI text.
        const CatInspection inspection =
            InspectCat(*g_saveData, cat);

        char text[1024];

        std::snprintf(
            text,
            sizeof(text),
            "CatManager | Cats: %zu | Living: %zu | Dead: %zu\n"
            "%s | %s | Level: %d | Age: %d\n"
            "Room: %s | %s%s\n"
            "Parents: %s / %s\n"
            "Children: %zu\n"
            "COI: %.6f",
            inspection.population.total,
            inspection.population.living,
            inspection.population.deceased,
            cat.name.c_str(),
            cat.sex.c_str(),
            cat.level,
            cat.age,
            cat.room.empty() ? "None" : cat.room.c_str(),
            cat.dead ? "Dead" : "Alive",
            cat.retired ? " | Retired" : "",
            inspection.parentA
                ? inspection.parentA->name.c_str()
                : "None",
            inspection.parentB
                ? inspection.parentB->name.c_str()
                : "None",
            inspection.children.size(),
            inspection.coi);

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

        Log(
            "[UI] CatManager button clicked: clicks=%u",
            static_cast<unsigned int>(g_buttonClicks));
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

        g_navigationReady = false;

        g_navigation.left_button = nullptr;
        g_navigation.right_button = nullptr;
        g_navigation.scene_manager = nullptr;
        g_navigation.text_synced = 0U;
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

        if (!sceneManager || !g_sceneReady)
            return;

        TrySetupButton();
        TrySetupNavigation(sceneManager);

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

    InitializeNavigationBinding();

    MewUI_InitSceneBinding(
        &g_houseScene,
        SCENE_NAME,
        SceneRefreshCallback,
        nullptr);

    MewUI_SetDebugLogsEnabled(false);

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