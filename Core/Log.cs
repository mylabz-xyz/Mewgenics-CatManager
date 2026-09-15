using System;
using System.IO;

namespace CatManager.Core;

public static class Log
{
    private static readonly string LogFile =
        Path.Combine(
            AppContext.BaseDirectory,
            "mod_logs",
            "catmanager.log"
        );

    public static void Info(string message)
    {
        Directory.CreateDirectory(
            Path.GetDirectoryName(LogFile)!
        );

        System.IO.File.AppendAllText(
            LogFile,
            $"[{DateTime.Now:HH:mm:ss}] {message}{Environment.NewLine}"
        );
    }
}