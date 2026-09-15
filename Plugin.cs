using CatManager.Core;

namespace CatManager;

public static class Plugin
{
    static Plugin()
    {
        Bootstrap.Initialize();
    }

    public static void Load()
    {
        Log.Info("Plugin.Load called");
    }
}