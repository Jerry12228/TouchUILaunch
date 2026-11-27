namespace Nte.Login;

// Keep all image-dependent layout and wire versions in the same verified
// profile. Supporting a new image requires choosing the complete contract.
internal sealed record NteOverseasSdkProfile(string ImageSha256, string BridgeSha256, NteSdkProtocol Protocol,
    int ConfigurationKeyRva, string AuthenticationVersion, string AuthenticationServerVersion, string DeviceStatisticsVersion);

internal sealed class NteOverseasRegion
{
    internal static NteOverseasRegion Global { get; } = new(NteRegionDefinition.Global, "com.hottagames.nte",
        "https://mapi.pwsdk.com", "global-sg", "pwsdk.com", "https://i.pwsdk.com", "Global", new(
        "0153111179C82EE2AA984B2DDB33648C692824A97A80AD104D522C742CF4E49B",
        "B922E40052ED1B0F7306D909986A69A58C1FDD869233307162DD4572A63D5AB2", NteSdkProtocol.Overseas208,
        0xF9AE90, "4.1.0", "20260917", "4.186.0"));
    internal static NteOverseasRegion Global209 { get; } = new(NteRegionDefinition.Global, "com.hottagames.nte",
        "https://mapi.pwsdk.com", "global-sg", "pwsdk.com", "https://i.pwsdk.com", "Global", new(
        "71F809AA59EDD26881B71C89002FEA57C62B31A9D8295ABC4E17BF40D3CF2F6C",
        "1D6EFB3600C46839015363F3CE3B31EB9BEB92668A54DA2A76480BB63FB00DDE", NteSdkProtocol.Overseas209,
        0xFAD728, "4.1.0", "20260917", "4.186.0"));

    internal static NteOverseasRegion ForInstallation(NteHostRegion region, string root)
    {
        var baseRegion = From(region);
        string platform = System.IO.Path.Combine(root, baseRegion.LauncherDirectory);
        string imageHash = Hash(System.IO.Path.Combine(platform, baseRegion.Installation.LauncherExecutable));
        string bridgeHash = Hash(System.IO.Path.Combine(platform, "WPGameClientSDK_64.dll"));
        return Resolve(region, imageHash, bridgeHash);
    }

    internal static NteOverseasRegion Resolve(NteHostRegion region, string imageHash, string bridgeHash)
    {
        foreach (var candidate in new[] { Global, Global209, Taiwan })
            if (candidate.HostRegion == region && candidate.ImageSha256.Equals(imageHash, StringComparison.OrdinalIgnoreCase) &&
                candidate.BridgeSha256.Equals(bridgeHash, StringComparison.OrdinalIgnoreCase)) return candidate;
        throw new NteGlobalLaunchException("GlobalSdkUnsupported");
    }

    private static string Hash(string path)
    {
        if (!System.IO.File.Exists(path)) throw new NteGlobalLaunchException("GlobalFilesMissing");
        using var stream = System.IO.File.OpenRead(path);
        return Convert.ToHexString(System.Security.Cryptography.SHA256.HashData(stream));
    }

    internal static NteOverseasRegion Taiwan { get; } = new(NteRegionDefinition.Taiwan, "tw.com.iwplay.nte",
        "https://mapitw.playcomb.com", "iwplay", "playcomb.com", "https://i.playcomb.com", null, new(
        "E853AC6797F9D2AC901FB76E3209F3D5841A4CA642017BD7182E6B8D26028C6A",
        "999551DF1512D6EE453B1A2C730A45893E5431B164157567A44F9B3FE1A04335", NteSdkProtocol.Overseas208,
        0xF9AE90, "4.1.0", "20260917", "4.186.0"));

    internal NteRegionDefinition Installation { get; }
    internal NteHostRegion HostRegion => Installation.Region;
    internal string Name => HostRegion.ToString();
    internal int ApplicationId => Installation.ApplicationId;
    internal string AppId => Installation.AppId;
    internal string LauncherDirectory => Installation.LauncherDirectory;
    internal string PackageName { get; }
    internal Uri Service { get; }
    internal string Area { get; }
    internal string BrowserDomain { get; }
    internal Uri WebService { get; }
    internal string? SavedDirectorySuffix { get; }
    internal NteOverseasSdkProfile Sdk { get; }
    internal string ImageSha256 => Sdk.ImageSha256;
    internal string BridgeSha256 => Sdk.BridgeSha256;
    internal NteSdkProtocol Protocol => Sdk.Protocol;
    internal Uri TermsUri => new(HostRegion == NteHostRegion.Taiwan
        ? "https://www.iwplay.com.tw/pdf/contract-ht.pdf"
        : "https://static.pwsdk.com/nte/contract/contract.html");
    internal Uri PrivacyUri => new(HostRegion == NteHostRegion.Taiwan
        ? "https://www.iwplay.com.tw/pdf/privacy-ht.pdf"
        : "https://static.pwsdk.com/nte/privacy/privacy.html");

    private NteOverseasRegion(NteRegionDefinition installation, string packageName,
        string service, string area, string browserDomain, string webService, string? savedDirectorySuffix,
        NteOverseasSdkProfile sdk)
    {
        Installation = installation; PackageName = packageName;
        Service = new(service); Area = area; BrowserDomain = browserDomain; WebService = new(webService);
        SavedDirectorySuffix = savedDirectorySuffix; Sdk = sdk;
    }

    internal static NteOverseasRegion From(NteHostRegion region) => region switch
    {
        NteHostRegion.Global => Global, NteHostRegion.Taiwan => Taiwan,
        _ => throw new ArgumentException("An overseas launch region is required.", nameof(region))
    };

    internal bool IsBrowserOrigin(Uri uri) => uri.Scheme == "https" && uri.IsDefaultPort && uri.UserInfo.Length == 0 &&
        uri.Fragment.Length == 0 && (uri.Host.Equals(BrowserDomain, StringComparison.OrdinalIgnoreCase) ||
            uri.Host.EndsWith("." + BrowserDomain, StringComparison.OrdinalIgnoreCase));
}
