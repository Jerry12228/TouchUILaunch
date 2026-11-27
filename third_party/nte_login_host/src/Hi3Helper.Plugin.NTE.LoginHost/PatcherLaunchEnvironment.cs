using System.Globalization;
using System.Text.Json;
using Nte.Login;

namespace Nte.LoginHost;

// The official client observed after returning to its title screen has these
// public paths. Both regions validate their own installed Patcher config;
// the game's SDK remains responsible for initialization, updates and IO.
internal static class PatcherLaunchEnvironment
{
    internal static bool TryApply(IDictionary<string, string?> environment, string gameRoot, string applicationId,
        string launcherDirectoryName = NteRegionDefinition.CnLauncherDirectory)
    {
        foreach (string name in new[] { "_PATCHER_CONFIG_PATH_", "_PATCHER_USERDATA_PATH_", "_PATCHER_LOG_PATH_" })
            environment.Remove(name);

        NteRegionDefinition definition;
        try { definition = NteRegionDefinition.FromLauncherDirectory(launcherDirectoryName); }
        catch (ArgumentException) { return false; }
        if (applicationId != definition.AppId) return false;
        uint expectedId = checked((uint)definition.ApplicationId);

        string launcher = Path.Combine(Path.GetFullPath(gameRoot), launcherDirectoryName);
        string config = Path.Combine(launcher, "ResFilesM", applicationId, "PatcherConfig");
        string userData = Path.Combine(launcher, "UserData");
        string configFile = Path.Combine(config, "PatcherConfig.json");
        if (!Directory.Exists(userData) || !File.Exists(Path.Combine(config, "PatcherPath.json")) ||
            !File.Exists(configFile))
            return false;

        try
        {
            using FileStream stream = File.OpenRead(configFile);
            if (stream.Length > 1024 * 1024) return false;
            using JsonDocument json = JsonDocument.Parse(stream);
            if (json.RootElement.ValueKind != JsonValueKind.Object ||
                !json.RootElement.TryGetProperty("appId", out JsonElement id))
                return false;
            bool matches = id.ValueKind switch
            {
                JsonValueKind.String => uint.TryParse(id.GetString(), NumberStyles.None, CultureInfo.InvariantCulture,
                    out uint value) && value == expectedId,
                JsonValueKind.Number => id.TryGetUInt32(out uint value) && value == expectedId,
                _ => false
            };
            if (!matches) return false;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException)
        {
            return false;
        }

        environment["_PATCHER_CONFIG_PATH_"] = config + Path.DirectorySeparatorChar;
        environment["_PATCHER_USERDATA_PATH_"] = userData + Path.DirectorySeparatorChar;
        environment["_PATCHER_LOG_PATH_"] = Path.Combine(userData, "Log", "Patcher", "log", "Game");
        return true;
    }
}
