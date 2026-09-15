namespace CatManager.Core;

public static class Bootstrap
{
    public static void Initialize()
    {
        Log.Info(ModInfo.Name);
        Log.Info($"Version {ModInfo.Version}");
        Log.Info("Bootstrap initialized");
    }
}