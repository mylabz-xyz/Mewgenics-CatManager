#include "CatManagerUI.h"
#include "CatManagerState.h"
#include "CatManagerSearch.h"
#include "CatManagerView.h"
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

    CatManagerState g_state{};

    MewUISceneBinding g_houseScene{};

    void *g_catManagerButton = nullptr;

    bool g_sceneReady = false;
    bool g_buttonReady = false;
    bool g_textReady = false;

    uint32_t g_buttonClicks = 0U;

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
    void ResetSceneState();

    void SetBreedingCatA(uint64_t catId);
    void SetBreedingCatB(uint64_t catId);

    std::string GetParentName(int64_t sqlKey);
    std::string BuildCatDetails(const CatData &cat);

    void TrySetupButton();

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

    void SetBreedingCatA(uint64_t catId)
    {
        if (!g_state.saveData)
            return;

        if (g_state.saveData->cats.find(catId) ==
            g_state.saveData->cats.end())
            return;

        g_state.breedingCatAId = catId;
    }

    void SetBreedingCatB(uint64_t catId)
    {
        if (!g_state.saveData)
            return;

        if (g_state.saveData->cats.find(catId) ==
            g_state.saveData->cats.end())
            return;

        g_state.breedingCatBId = catId;
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
        static_cast<uint32_t>(g_state.selectedCatIndex),
        CatManagerNavigationChangedCallback,
        nullptr);
}

void RebuildNavigationValues()
{
    g_navigationValues.clear();
    g_navigationValuePointers.clear();

    if (!g_state.saveData)
        return;

    g_navigationValuePointers.reserve(
        g_state.catOrder.size());

    for (const uint64_t catId :
         g_state.catOrder)
    {
        const auto it =
            g_state.saveData->cats.find(catId);

        if (it == g_state.saveData->cats.end())
            continue;

        g_navigationValues.push_back(
            it->second.name);
    }

    for (const std::string &value :
         g_navigationValues)
    {
        g_navigationValuePointers.push_back(
            value.c_str());
    }
}

void __cdecl CatManagerNavigationChangedCallback(
    MewUINavigationBinding *binding,
    uint32_t index,
    const char *value,
    void *userData)
{
    (void)userData;

    if (!binding ||
        !g_state.saveData ||
        g_state.catOrder.empty())
        return;

    if (index >= g_state.catOrder.size())
        return;

    const uint64_t catId =
        g_state.catOrder[index];

    Log(
        "[UI] Cat navigation: index=%u/%zu value='%s'",
        static_cast<unsigned int>(index + 1U),
        g_state.catOrder.size(),
        value ? value : "");

    if (SelectLivingCat(g_state, catId))
        RenderSelectedCat(
            g_state,
            g_textReady);
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
        static_cast<uint32_t>(g_state.selectedCatIndex));

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
        if (!g_state.saveData || sqlKey <= 0)
            return "None";

        auto sqlIt = g_state.saveData->sqlToCat.find(sqlKey);

        if (sqlIt == g_state.saveData->sqlToCat.end())
            return "Historical #" + std::to_string(sqlKey);

        auto catIt = g_state.saveData->cats.find(sqlIt->second);

        if (catIt == g_state.saveData->cats.end())
            return "Historical #" + std::to_string(sqlKey);

        return catIt->second.name;
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

        RenderSelectedCat(
            g_state,
            g_textReady);
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
            UpdateCatManagerText(
                g_state,
                g_textReady);
    }
}

// ============================================================
// Public API
// ============================================================

void CatManagerUI_Init(const SaveData *saveData)
{
    Log("[UI] CatManagerUI_Init");

    g_state = {};
    g_state.saveData = saveData;

    g_buttonClicks = 0U;
    g_state.selectedCatIndex = 0U;

    g_state.searchQuery.clear();
    g_state.searchResults.clear();
    g_state.selectedSearchIndex = 0U;

    g_state.breedingCatAId = 0U;
    g_state.breedingCatBId = 0U;

    g_state.currentView = CatManagerView::Closed;

    ResetSceneState();
    RebuildCatOrder(g_state);
    RebuildNavigationValues();

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

    g_state = {};
    g_state.saveData = saveData;
    g_state.currentView =
        CatManagerView::Closed;

    g_state.selectedCatIndex = 0U;

    g_state.searchQuery.clear();
    g_state.searchResults.clear();
    g_state.selectedSearchIndex = 0U;

    g_state.breedingCatAId = 0U;
    g_state.breedingCatBId = 0U;

    g_state.currentView = CatManagerView::Closed;

    RebuildCatOrder(g_state);
    RebuildNavigationValues();

    // Si l'UI existe déjà, on rafraîchit immédiatement.
    if (g_buttonReady)
    {
        g_textReady = false;

        RenderSelectedCat(
            g_state,
            g_textReady);
    }
}

void CatManagerUI_Shutdown()
{
    Log("[UI] CatManagerUI_Shutdown");
    g_state = {};

    ResetSceneState();

    MewUI_ClearSceneBinding(&g_houseScene);
    MewUI_Stop();

    g_state.saveData = nullptr;

    g_state.catOrder.clear();
    g_state.selectedCatIndex = 0U;

    g_state.searchQuery.clear();
    g_state.searchResults.clear();
    g_state.selectedSearchIndex = 0U;

    g_state.breedingCatAId = 0U;
    g_state.breedingCatBId = 0U;

    g_state.currentView = CatManagerView::Closed;

    g_buttonClicks = 0U;
}