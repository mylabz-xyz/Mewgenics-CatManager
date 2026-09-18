#include "CatManagerUI.h"
#include "CatManagerState.h"
#include "CatManagerSearch.h"
#include "CatManagerView.h"
#include "mew_ui_api.h"
#include "../Logger.h"
#include "../Save/Analysis/BreedingAnalyzer.h"
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

    constexpr const char *BREEDING_A_LEFT_NODE_NAME = "breeding_a_left";
    constexpr const char *BREEDING_A_RIGHT_NODE_NAME = "breeding_a_right";
    constexpr const char *BREEDING_A_VALUE_NODE_NAME = "breeding_cat_a";
    constexpr const char *BREEDING_A_LEFT_ROLE_NAME = "CatManager_Breeding_A_Left";
    constexpr const char *BREEDING_A_RIGHT_ROLE_NAME = "CatManager_Breeding_A_Right";

    constexpr const char *BREEDING_B_LEFT_NODE_NAME = "breeding_b_left";
    constexpr const char *BREEDING_B_RIGHT_NODE_NAME = "breeding_b_right";
    constexpr const char *BREEDING_B_VALUE_NODE_NAME = "breeding_cat_b";
    constexpr const char *BREEDING_B_LEFT_ROLE_NAME = "CatManager_Breeding_B_Left";
    constexpr const char *BREEDING_B_RIGHT_ROLE_NAME = "CatManager_Breeding_B_Right";

    constexpr const char *BREEDING_VALUE_TEXT_KEY = "CATMANAGER_NAV_VAL";
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

    MewUINavigationBinding g_breedingNavigationA{};
    MewUINavigationBinding g_breedingNavigationB{};

    bool g_breedingNavigationAReady = false;
    bool g_breedingNavigationBReady = false;

    std::vector<std::string> g_navigationValues;
    std::vector<const char *> g_navigationValuePointers;

}

// ============================================================
// Forward declarations
// ============================================================

namespace
{
    void ResetSceneState();

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

uint32_t FindCatNavigationIndex(
    const CatManagerState &state,
    uint64_t catId)
{
    if (catId == 0U)
        return 0U;

    for (size_t i = 0U; i < state.catOrder.size(); ++i)
    {
        if (state.catOrder[i] == catId)
            return static_cast<uint32_t>(i);
    }

    return 0U;
}

void __cdecl CatManagerBreedingNavigationBChangedCallback(
    MewUINavigationBinding *binding,
    uint32_t index,
    const char *value,
    void *userData)
{
    (void)userData;

    if (!binding ||
        !g_state.saveData ||
        index >= g_state.catOrder.size())
    {
        return;
    }

    const uint64_t catId =
        g_state.catOrder[index];

    if (catId == g_state.breedingCatAId)
    {
        Log(
            "[UI] Breeding B ignored: same cat as A");
        return;
    }

    g_state.breedingCatBId = catId;
    g_state.currentView =
        CatManagerView::Breeding;

    g_textReady = false;

    Log(
        "[UI] Breeding B navigation: index=%u value='%s' id=%llu",
        static_cast<unsigned int>(index),
        value ? value : "",
        static_cast<unsigned long long>(catId));
}

void __cdecl CatManagerBreedingNavigationAChangedCallback(
    MewUINavigationBinding *binding,
    uint32_t index,
    const char *value,
    void *userData)
{
    (void)userData;

    if (!binding ||
        !g_state.saveData ||
        index >= g_state.catOrder.size())
    {
        return;
    }

    const uint64_t catId =
        g_state.catOrder[index];

    if (catId == g_state.breedingCatBId)
    {
        Log(
            "[UI] Breeding A ignored: same cat as B");
        return;
    }

    g_state.breedingCatAId = catId;
    g_state.currentView =
        CatManagerView::Breeding;

    g_textReady = false;

    Log(
        "[UI] Breeding A navigation: index=%u value='%s' id=%llu",
        static_cast<unsigned int>(index),
        value ? value : "",
        static_cast<unsigned long long>(catId));
}

void TrySetupBreedingNavigation(void *sceneManager)
{
    if (!sceneManager ||
        !g_state.saveData ||
        g_navigationValuePointers.empty())
    {
        return;
    }

    if (!g_breedingNavigationAReady)
    {
        g_breedingNavigationA.scene_name =
            SCENE_NAME;
        g_breedingNavigationA.left_node_name =
            BREEDING_A_LEFT_NODE_NAME;
        g_breedingNavigationA.right_node_name =
            BREEDING_A_RIGHT_NODE_NAME;
        g_breedingNavigationA.value_node_name =
            BREEDING_A_VALUE_NODE_NAME;
        g_breedingNavigationA.left_role_name =
            BREEDING_A_LEFT_ROLE_NAME;
        g_breedingNavigationA.right_role_name =
            BREEDING_A_RIGHT_ROLE_NAME;
        g_breedingNavigationA.value_text_key =
            BREEDING_VALUE_TEXT_KEY;
        g_breedingNavigationA.values =
            g_navigationValuePointers.data();
        g_breedingNavigationA.value_count =
            static_cast<uint32_t>(
                g_navigationValuePointers.size());
        g_breedingNavigationA.index =
            FindCatNavigationIndex(
                g_state,
                g_state.breedingCatAId);
        g_breedingNavigationA.changed_callback =
            CatManagerBreedingNavigationAChangedCallback;
        g_breedingNavigationA.user_data = nullptr;

        int created = 0;

        if (MewUI_SetupNavigationInScene(
                &g_breedingNavigationA,
                sceneManager,
                &created))
        {
            g_breedingNavigationAReady = true;

            MewUI_SetNavigationIndex(
                &g_breedingNavigationA,
                g_breedingNavigationA.index);

            Log(
                "[UI] Breeding A navigation ready: created=%d",
                created);
        }
    }

    if (!g_breedingNavigationBReady)
    {
        g_breedingNavigationB.scene_name =
            SCENE_NAME;
        g_breedingNavigationB.left_node_name =
            BREEDING_B_LEFT_NODE_NAME;
        g_breedingNavigationB.right_node_name =
            BREEDING_B_RIGHT_NODE_NAME;
        g_breedingNavigationB.value_node_name =
            BREEDING_B_VALUE_NODE_NAME;
        g_breedingNavigationB.left_role_name =
            BREEDING_B_LEFT_ROLE_NAME;
        g_breedingNavigationB.right_role_name =
            BREEDING_B_RIGHT_ROLE_NAME;
        g_breedingNavigationB.value_text_key =
            BREEDING_VALUE_TEXT_KEY;
        g_breedingNavigationB.values =
            g_navigationValuePointers.data();
        g_breedingNavigationB.value_count =
            static_cast<uint32_t>(
                g_navigationValuePointers.size());
        g_breedingNavigationB.index =
            FindCatNavigationIndex(
                g_state,
                g_state.breedingCatBId);
        g_breedingNavigationB.changed_callback =
            CatManagerBreedingNavigationBChangedCallback;
        g_breedingNavigationB.user_data = nullptr;

        int created = 0;

        if (MewUI_SetupNavigationInScene(
                &g_breedingNavigationB,
                sceneManager,
                &created))
        {
            g_breedingNavigationBReady = true;

            MewUI_SetNavigationIndex(
                &g_breedingNavigationB,
                g_breedingNavigationB.index);

            Log(
                "[UI] Breeding B navigation ready: created=%d",
                created);
        }
    }
}

void __cdecl CatManagerNavigationChangedCallback(
    MewUINavigationBinding *binding,
    uint32_t index,
    const char *value,
    void *userData);

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

    g_navigationValues.reserve(
        g_state.catOrder.size());

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

    void SelectCurrentCatForBreeding()
    {
        if (!g_state.saveData)
            return;

        const CatData *cat =
            GetSelectedCat(g_state);

        if (!cat)
            return;

        if (g_state.breedingCatAId == 0U)
        {
            g_state.breedingCatAId = cat->id;
            g_state.currentView =
                CatManagerView::Breeding;

            g_textReady = false;

            Log(
                "[UI] Breeding A selected: '%s' id=%llu",
                cat->name.c_str(),
                static_cast<unsigned long long>(cat->id));

            return;
        }

        if (g_state.breedingCatBId == 0U)
        {
            if (cat->id == g_state.breedingCatAId)
            {
                Log(
                    "[UI] Breeding B selection ignored: same cat as A");
                return;
            }

            g_state.breedingCatBId = cat->id;
            g_textReady = false;

            Log(
                "[UI] Breeding B selected: '%s' id=%llu",
                cat->name.c_str(),
                static_cast<unsigned long long>(cat->id));

            return;
        }

        Log(
            "[UI] Breeding selection complete: A=%llu B=%llu",
            static_cast<unsigned long long>(
                g_state.breedingCatAId),
            static_cast<unsigned long long>(
                g_state.breedingCatBId));
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

        if (g_state.currentView == CatManagerView::Closed)
        {
            g_state.currentView =
                CatManagerView::Search;

            g_state.searchQuery.clear();
            g_state.searchResults.clear();
            g_state.selectedSearchIndex = 0U;

            g_state.breedingCatAId = 0U;
            g_state.breedingCatBId = 0U;

            g_textReady = false;

            Log(
                "[UI] CatManager opened: clicks=%u",
                static_cast<unsigned int>(
                    g_buttonClicks));

            return;
        }

        SelectCurrentCatForBreeding();

        Log(
            "[UI] CatManager opened: clicks=%u",
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
        TrySetupBreedingNavigation(sceneManager);

        if (!g_buttonReady || g_textReady)
            return;

        switch (g_state.currentView)
        {
        case CatManagerView::Closed:
            return;

        case CatManagerView::Search:
            UpdateCatManagerText(g_state, g_textReady);
            return;

        case CatManagerView::Breeding:
            UpdateBreedingSelectionText(g_state, g_textReady);
            return;
        }
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

void RunBreedingAnalysis()
{
    if (!g_state.saveData)
        return;

    const CatData *catA =
        FindCat(*g_state.saveData, g_state.breedingCatAId);

    const CatData *catB =
        FindCat(*g_state.saveData, g_state.breedingCatBId);

    if (!catA || !catB)
    {
        Log("[UI] Breeding analysis: invalid cats");
        return;
    }

    const BreedingAnalysis analysis =
        AnalyzeBreeding(
            *g_state.saveData,
            *catA,
            *catB,
            10U);

    Log(
        "[UI] Breeding analysis: '%s' + '%s' -> COI %.4f%%",
        catA->name.c_str(),
        catB->name.c_str(),
        analysis.expectedOffspringCoi * 100.0);

    for (const BreedingCommonAncestor &common :
         analysis.commonAncestors)
    {
        if (!common.ancestor)
            continue;

        Log(
            "[UI] Common ancestor: '%s' depthA=%zu depthB=%zu contribution=%.4f%%",
            common.ancestor->name.c_str(),
            common.depthA,
            common.depthB,
            common.contribution * 100.0);
    }
}