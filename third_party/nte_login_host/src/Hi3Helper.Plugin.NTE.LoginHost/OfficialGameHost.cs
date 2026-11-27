using System.Diagnostics;
using Hi3Helper.Plugin.NTE.Management.Game;
using Nte.Login;

namespace Nte.LoginHost;

internal static class OfficialGameHost
{
    internal static async Task<bool> RunAsync(NteHostLaunchOptions options)
    {
        NteOfficialLaunchPlan plan = NteOfficialLaunchPlan.Create(options.GameRoot, options.Region);
        using Process platform = StartPlatform(plan, options.AdditionalArguments)
                                 ?? throw new InvalidOperationException("Official launcher could not start.");
        // The official platform owns authentication and the game. Observe the
        // selected installation only; never read or import its saved accounts.
        Stopwatch startup = Stopwatch.StartNew();
        while (startup.Elapsed < TimeSpan.FromMinutes(3))
        {
            foreach (Process candidate in Process.GetProcessesByName("HTGame"))
            {
                using (candidate)
                {
                    if (!NteProcessImage.Matches(candidate.Id, plan.GameExecutable)) continue;
                    await NteProcessImage.WaitForExitWithLimitedAccessAsync(candidate.Id, plan.GameExecutable,
                        CancellationToken.None);
                    return true;
                }
            }

            if (platform.HasExited && platform.ExitCode != 0) return false;
            await Task.Delay(500);
        }

        return false;
    }

    private static Process? StartPlatform(NteOfficialLaunchPlan plan, string? arguments)
    {
        try
        {
            return Process.Start(plan.CreateStartInfo(arguments));
        }
        catch (System.ComponentModel.Win32Exception error) when (error.NativeErrorCode == 740)
        {
            // Overseas official platforms requireAdministrator. Ask Windows to
            // apply that manifest when the parent launcher is not elevated.
            return Process.Start(plan.CreateStartInfo(arguments, requestElevation: true));
        }
    }
}
