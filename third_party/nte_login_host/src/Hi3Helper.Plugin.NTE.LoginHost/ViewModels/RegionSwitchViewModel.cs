using Nte.Login;
using System;
using System.Globalization;
using System.Threading;
using System.Threading.Tasks;

namespace Nte.LoginHost.ViewModels;

internal sealed class RegionSwitchViewModel : ObservableObject, IDisposable
{
    private readonly INteRegionSwitchClient client;
    private readonly NteRegionSwitchOptions options;
    private readonly Func<string, string> text;
    private bool busy, ready, completed;
    private string status = "", detail = "", summary = "";
    private double progressValue;
    public string CheckLabel => text("ServerSwitchCheck");
    public string SwitchLabel => text("ServerSwitchApply");
    public string CancelLabel => text("ServerSwitchCancel");
    public string Hint => text("ServerSwitchHint");
    public string Title => text("ServerSwitchTitle");
    public string TargetName => text(options.Target == NteHostRegion.Taiwan ? "ServerTaiwan" : "ServerGlobal");
    public string InstallationPath => options.GameRoot;

    public bool IsBusy
    {
        get => busy;
        private set
        {
            if (Set(ref busy, value)) RefreshCommands();
        }
    }

    public bool IsCompleted
    {
        get => completed;
        private set
        {
            Set(ref completed, value);
            RefreshCommands();
        }
    }

    public string Status
    {
        get => status;
        private set => Set(ref status, value);
    }

    public string Detail
    {
        get => detail;
        private set => Set(ref detail, value);
    }

    public string Summary
    {
        get => summary;
        private set => Set(ref summary, value);
    }

    public double ProgressValue
    {
        get => progressValue;
        private set => Set(ref progressValue, value);
    }

    public AsyncRelayCommand CheckCommand { get; }
    public AsyncRelayCommand SwitchCommand { get; }
    public RelayCommand CancelCommand { get; }
    internal event Action? Completed;

    internal RegionSwitchViewModel(INteRegionSwitchClient client, NteRegionSwitchOptions options,
        Func<string, string> text)
    {
        this.client = client;
        this.options = options;
        this.text = text;
        CheckCommand = new(CheckAsync, () => !IsBusy && !IsCompleted);
        SwitchCommand = new(SwitchAsync, () => ready && !IsBusy && !IsCompleted);
        CancelCommand = new(() =>
        {
            CheckCommand.Cancel();
            SwitchCommand.Cancel();
        }, () => IsBusy);
    }

    private async Task CheckAsync(CancellationToken token)
    {
        ready = false;
        Summary = "";
        await RunAsync(async progress =>
        {
            NteSwitchSummary plan = await client.PlanAsync(options, progress, token);
            ready = true;
            Summary = plan.RecoveryRequired
                ? text("ServerSwitchRecovery")
                : string.Format(CultureInfo.CurrentCulture, text("ServerSwitchSummary"), plan.TargetVersion,
                    plan.ReusedFiles, plan.CachedFiles, plan.DownloadFiles, plan.DownloadBytes / 1000000000d);
            Status = text("ServerSwitchReady");
        });
    }

    private async Task SwitchAsync(CancellationToken token)
    {
        await RunAsync(async progress =>
        {
            NteSwitchSummary result = await client.SwitchAsync(options, progress, token);
            IsCompleted = result.Completed;
            if (!result.Completed) throw new InvalidOperationException(text("ServerSwitchFailed"));
            ProgressValue = 100;
            Status = text("ServerSwitchCompleted");
            Detail = text("ServerSwitchCompletedHint");
            Completed?.Invoke();
        });
    }

    private async Task RunAsync(Func<IProgress<NteSwitchProgress>, Task> action)
    {
        IsBusy = true;
        Status = text("ServerSwitchPreparing");
        Detail = "";
        IProgress<NteSwitchProgress> progress = new Progress<NteSwitchProgress>(value =>
        {
            ProgressValue = value.Total > 0 ? Math.Clamp(value.Processed * 100d / value.Total, 0, 100) : 0;
            Status = text(value.Phase switch
            {
                1 => "ServerSwitchChecking", 2 => "ServerSwitchDownloading", 3 => "ServerSwitchActivating",
                _ => "ServerSwitchPreparing"
            });
            Detail = value.Path;
        });
        try
        {
            await action(progress);
        }
        catch (OperationCanceledException)
        {
            Status = text("ServerSwitchCancelled");
            Detail = text("ServerSwitchCancelledHint");
        }
        catch (Exception error)
        {
            Status = text("ServerSwitchFailed");
            Detail = error.Message;
        }
        finally
        {
            IsBusy = false;
        }
    }

    private void RefreshCommands()
    {
        CheckCommand?.Refresh();
        SwitchCommand?.Refresh();
        CancelCommand?.Refresh();
    }

    public void Dispose()
    {
        CheckCommand.Dispose();
        SwitchCommand.Dispose();
    }
}
