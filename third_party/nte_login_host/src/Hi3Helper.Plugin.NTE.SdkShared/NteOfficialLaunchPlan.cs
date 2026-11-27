using System;
using System.Diagnostics;
using System.IO;

namespace Nte.Login;

internal sealed record NteOfficialLaunchPlan(string Executable, string GameExecutable)
{
    internal static NteOfficialLaunchPlan Create(string gameRoot, bool global)
        => Create(gameRoot, global ? NteHostRegion.Global : NteHostRegion.CN);

    internal static NteOfficialLaunchPlan Create(string gameRoot, NteHostRegion region)
    {
        string root = Path.GetFullPath(gameRoot);
        NteRegionDefinition definition = NteRegionDefinition.From(region);
        string executable = Path.Combine(root, definition.LauncherDirectory, definition.LauncherExecutable);
        if (!File.Exists(executable))
            throw new FileNotFoundException(
                "The official launcher must be installed inside the game installation directory.", executable);
        return new(executable,
            Path.Combine(root, NteRegionDefinition.GameExecutableRelativePath));
    }

    internal ProcessStartInfo CreateStartInfo(string? additionalArguments, bool requestElevation = false)
    {
        ProcessStartInfo start = new(Executable)
        {
            WorkingDirectory = Path.GetDirectoryName(Executable)!, UseShellExecute = requestElevation,
            Verb = requestElevation ? "runas" : "",
            CreateNoWindow = !requestElevation,
            WindowStyle = requestElevation ? ProcessWindowStyle.Normal : ProcessWindowStyle.Hidden
        };
        // ShellExecute is used only for the Windows elevation verb. The exact
        // executable and arguments are passed directly; no command shell runs.
        start.Arguments = "/launcher /directly /autoplay" +
                          (string.IsNullOrWhiteSpace(additionalArguments) ? "" : " " + additionalArguments);
        return start;
    }
}
