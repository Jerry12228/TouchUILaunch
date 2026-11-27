using Nte.Login;
namespace Nte.LoginHost.Services;
// Installed game files are never changed by the touch UI launcher.
internal sealed class NativeRegionSwitchClient : INteRegionSwitchClient
{
    public Task<NteSwitchSummary> PlanAsync(NteRegionSwitchOptions options, IProgress<NteSwitchProgress> progress, CancellationToken token)
        => Task.FromException<NteSwitchSummary>(new NotSupportedException("Region file switching is not part of TouchUILaunch."));
    public Task<NteSwitchSummary> SwitchAsync(NteRegionSwitchOptions options, IProgress<NteSwitchProgress> progress, CancellationToken token)
        => PlanAsync(options, progress, token);
}
