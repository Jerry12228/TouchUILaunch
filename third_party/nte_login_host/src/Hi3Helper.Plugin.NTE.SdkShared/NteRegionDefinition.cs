using System;
using System.Globalization;
using System.IO;

namespace Nte.Login;

// Public installation identity shared by the native plugin and LoginHost.
// SDK image/layout profiles are kept separately from these stable identities.
internal sealed class NteRegionDefinition
{
    internal const string CnAppId = "1289", GlobalAppId = "3000001", TaiwanAppId = "2000013";
    internal const string CnLauncherDirectory = "NTELauncher", GlobalLauncherDirectory = "NTEGlobal", TaiwanLauncherDirectory = "NTETW";
    internal const string RequiredGameArguments = "/Game/LoginAndCreate/Map/Updater/Updater_P -SAVEWINPOS=1";
    internal const string PatcherSdkRelativePath = "UserData/Patcher/PatcherSDK";
    internal static string GameExecutableRelativePath { get; } =
        Path.Combine("Client", "WindowsNoEditor", "HT", "Binaries", "Win64", "HTGame.exe");
    internal static NteRegionDefinition CN { get; } = new(NteHostRegion.CN, CnAppId, CnLauncherDirectory, "NTEGame.exe", 1);
    internal static NteRegionDefinition Global { get; } = new(NteHostRegion.Global, GlobalAppId, GlobalLauncherDirectory, "NTEGlobalGame.exe", 100);
    internal static NteRegionDefinition Taiwan { get; } = new(NteHostRegion.Taiwan, TaiwanAppId, TaiwanLauncherDirectory, "NTETWGame.exe", 100);

    internal NteHostRegion Region { get; }
    internal string AppId { get; }
    internal int ApplicationId => int.Parse(AppId, CultureInfo.InvariantCulture);
    internal string LauncherDirectory { get; }
    internal string LauncherExecutable { get; }
    internal uint LoginNotification { get; }
    internal bool IsOverseas => Region is NteHostRegion.Global or NteHostRegion.Taiwan;

    private NteRegionDefinition(NteHostRegion region, string appId, string launcher, string executable, uint notification)
    { Region = region; AppId = appId; LauncherDirectory = launcher; LauncherExecutable = executable; LoginNotification = notification; }

    internal static NteRegionDefinition From(NteHostRegion region) => region switch
    {
        NteHostRegion.CN => CN, NteHostRegion.Global => Global, NteHostRegion.Taiwan => Taiwan,
        _ => throw new ArgumentException("Unknown NTE region.", nameof(region))
    };
    internal static NteRegionDefinition FromLauncherDirectory(string directory) => directory switch
    {
        CnLauncherDirectory => CN, GlobalLauncherDirectory => Global, TaiwanLauncherDirectory => Taiwan,
        _ => throw new ArgumentException("Unknown NTE launcher directory.", nameof(directory))
    };
    internal static bool IsSupportedAppId(string appId) => appId is CnAppId or GlobalAppId or TaiwanAppId;
}
