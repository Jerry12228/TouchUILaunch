using System.Security.Cryptography;

namespace Nte.Login;

internal sealed class NteCnLaunchException(string key) : InvalidOperationException(key)
{
    internal string Key { get; } = key;
}

// Each launcher/bridge pair has a separately observed local IPC contract.
// A game resource version alone cannot identify the installed launcher SDK.
internal sealed record NteCnSdkCompatibility(string LauncherSha256, string BridgeSha256, NteSdkProtocol Protocol)
{
    // PC service protocol, independent of the bridge's local IPC version.
    internal const string ServiceSdkVersion = "5.159.0";
    internal string ProtocolVersion => Protocol.Version;
    private static readonly NteCnSdkCompatibility[] Profiles =
    [
        new("3CF58ECBE4CD33A83E741C12437CA9BB67678D166548C5972579C1266F250D9F",
            "4101B035406BECDA3EACBF0B2DABD7F43D9DF3F9294906706FC504623E7CD13B", NteSdkProtocol.Cn208),
        // Official launcher 1.0.7.1102 (ce6bd117), verified 2026-11-05.
        new("AF1C6D0DFF6B466E9ED0E1E1565DC26F72C4A1149151A53E749832226AB4FB1F",
            "AC9D9551810264594082BBD1704BCA588220DA087F4F8FB8600C41F0135E38C4", NteSdkProtocol.Cn209)
    ];

    internal static NteCnSdkCompatibility Resolve(string launcherSha256, string bridgeSha256)
        => Profiles.SingleOrDefault(p => p.LauncherSha256.Equals(launcherSha256, StringComparison.OrdinalIgnoreCase) &&
            p.BridgeSha256.Equals(bridgeSha256, StringComparison.OrdinalIgnoreCase)) ?? throw new NteCnLaunchException("CnUnsupportedSdk");

    internal static async Task<(NteCnSdkCompatibility Profile, string BridgePath)> LoadAsync(string gameRoot)
    {
        string executable = Path.Combine(gameRoot, NteRegionDefinition.GameExecutableRelativePath);
        string launcher = Path.Combine(gameRoot, NteRegionDefinition.CN.LauncherDirectory, NteRegionDefinition.CN.LauncherExecutable);
        string bridge = Path.Combine(gameRoot, NteRegionDefinition.CN.LauncherDirectory, "WPGameClientSDK_64.dll");
        foreach (string file in new[] { executable, launcher, bridge })
            if (!File.Exists(file)) throw new NteCnLaunchException("CnFilesMissing");
        return (Resolve(await HashAsync(launcher), await HashAsync(bridge)), bridge);
    }

    private static async Task<string> HashAsync(string path)
    {
        await using FileStream stream = File.OpenRead(path);
        return Convert.ToHexString(await SHA256.HashDataAsync(stream));
    }
}
