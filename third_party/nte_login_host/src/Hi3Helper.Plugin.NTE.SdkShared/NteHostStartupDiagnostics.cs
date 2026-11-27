using System.ComponentModel;
using System.Text.Json;

namespace Nte.Login;

internal static class NteHostStartupDiagnostics
{
    internal static string DefaultPath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "TouchUILaunch", "NTE", "Diagnostics", "last-startup.json");

    internal static void Phase(string phase, Exception? error = null)
    {
        if (phase.Length > 64 || phase.Any(c => !char.IsAsciiLetterOrDigit(c) && c != '-')) return;
        try
        {
            string path = Path.Combine(Path.GetDirectoryName(DefaultPath)!, "runtime.json");
            Directory.CreateDirectory(Path.GetDirectoryName(path)!);
            File.WriteAllText(path, JsonSerializer.Serialize(new Dictionary<string, object?>
        {
            ["utc"] = DateTimeOffset.UtcNow,
            ["hostPid"] = Environment.ProcessId,
            ["hostVersion"] = typeof(NteHostStartupDiagnostics).Assembly.GetName().Version?.ToString(),
            ["phase"] = phase,
            ["exceptionType"] = error?.GetType().Name
        }, NteJsonContext.Default.DictionaryStringObject));
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException) { }
    }

    // Exception messages, stack traces, arguments and SDK frames can contain credentials.
    // Persist only fixed failure identifiers and numeric native status.
    internal static void TryWrite(string path, Exception error, string key)
    {
        try
        {
            if (key.Length > 64 || key.Any(c => !char.IsAsciiLetterOrDigit(c))) return;
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
            File.WriteAllText(path, JsonSerializer.Serialize(new Dictionary<string, object?>
        {
            ["utc"] = DateTimeOffset.UtcNow,
            ["hostVersion"] = typeof(NteHostStartupDiagnostics).Assembly.GetName().Version?.ToString(),
            ["key"] = key,
            ["exceptionType"] = error.GetType().Name,
            ["nativeErrorCode"] = error is Win32Exception native ? native.NativeErrorCode : (int?)null
        }, NteJsonContext.Default.DictionaryStringObject));
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or ArgumentException) { }
    }
}
