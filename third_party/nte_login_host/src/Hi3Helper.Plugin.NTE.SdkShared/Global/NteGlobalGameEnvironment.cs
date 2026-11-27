using System.Diagnostics;
using System.Text;

namespace Nte.Login;

// Application bootstrap consumed and cleared by the game's installed SDK.
// This is separate from Arc's host handshake and from channel IPC frames.
internal static class NteGlobalGameEnvironment
{
    internal static IReadOnlyList<string> Keys { get; } =
        ["GAME_APPID", "GAME_APPKEY", "GAME_AREA", "chatCoreDomain", "chatTCPDomain", "chatIMApiDomain"];

    internal static void Apply(ProcessStartInfo start, NteOverseasApplicationConfiguration configuration)
    {
        Clear(start);
        start.Environment["GAME_APPID"] = configuration.Region.AppId;
        start.Environment["GAME_APPKEY"] = Encoding.UTF8.GetString(configuration.ApplicationKey);
        start.Environment["GAME_AREA"] = configuration.GameArea;
        foreach (string name in Keys.Skip(3))
            if (configuration.GameServices.TryGetValue(name, out string? value)) start.Environment[name] = value;
    }

    // The parent needs no copy of these strings after Process.Start, including
    // when creating the child fails. The SDK separately clears its own copy.
    internal static void Clear(ProcessStartInfo start)
    {
        foreach (string key in Keys) start.Environment.Remove(key);
    }
}
