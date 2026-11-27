using System;
using System.Collections.Generic;
using System.IO;

namespace Nte.Login;

internal sealed record NteRegionSwitchOptions(string GameRoot, NteHostRegion Target, string Language = "zh-CN")
{
    internal string TargetAppId => NteRegionDefinition.From(Target) is { IsOverseas: true } definition
        ? definition.AppId
        : throw new ArgumentException("Only Taiwan and Global can share a game installation.");
    internal string[] ToArguments()
    {
        Validate();
        return ["--switch-region", Path.GetFullPath(GameRoot), "--target-region", Target.ToString(), "--language", Language];
    }
    internal static NteRegionSwitchOptions Parse(IReadOnlyList<string> arguments)
    {
        if (arguments.Count is not (4 or 6) || arguments[0] != "--switch-region" || arguments[2] != "--target-region" ||
            arguments[3] is not ("Global" or "Taiwan") || arguments.Count == 6 && arguments[4] != "--language") throw new ArgumentException("Invalid server switch arguments.");
        NteRegionSwitchOptions options = new(arguments[1], Enum.Parse<NteHostRegion>(arguments[3]), arguments.Count == 6 && arguments[4] == "--language" ? arguments[5] : "zh-CN");
        options.Validate(); return options;
    }
    private void Validate()
    {
        if (!Path.IsPathFullyQualified(GameRoot) || GameRoot.Contains('\0')) throw new ArgumentException("Select an absolute NTE installation path.");
        if (Language.Length is < 1 or > 35 || System.Linq.Enumerable.Any(Language, c => !char.IsAsciiLetterOrDigit(c) && c is not ('-' or '_'))) throw new ArgumentException("Invalid language.");
        _ = TargetAppId;
    }
}

internal sealed record NteSwitchProgress(int Phase, long Processed, long Total, int Files, int Count, string Path);
internal sealed record NteSwitchSummary(string SourceAppId, string TargetAppId, string TargetVersion,
    int ReusedFiles, int CachedFiles, int DownloadFiles, long DownloadBytes, bool Completed, bool RecoveryRequired);

internal interface INteRegionSwitchClient
{
    System.Threading.Tasks.Task<NteSwitchSummary> PlanAsync(NteRegionSwitchOptions options, IProgress<NteSwitchProgress> progress, System.Threading.CancellationToken token);
    System.Threading.Tasks.Task<NteSwitchSummary> SwitchAsync(NteRegionSwitchOptions options, IProgress<NteSwitchProgress> progress, System.Threading.CancellationToken token);
}
