using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;
using Avalonia.Threading;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;
using Nte.Login;

namespace Nte.LoginHost;

public partial class App : Application
{
    private LoginWindow? window;
    private IClassicDesktopStyleApplicationLifetime lifetime = null!;

    public override void Initialize() => AvaloniaXamlLoader.Load(this);

    public override void OnFrameworkInitializationCompleted()
    {
        base.OnFrameworkInitializationCompleted();
        lifetime = (IClassicDesktopStyleApplicationLifetime)ApplicationLifetime!;
        string[] arguments = lifetime.Args ?? [];

        if (arguments.Length == 2 && arguments[0] == "--verify-ui")
        {
            _ = VerifyUiAsync(arguments[1]);
            return;
        }

        try
        {
            Localization.LoginText.UseLanguage(
                arguments.Length == 4 && arguments[0] is "--preview-global-pid" or "--preview-taiwan-pid"
                    ? arguments[3]
                    : NteHostLaunchOptions.Parse(arguments).Language);
        }
        catch (ArgumentException)
        {
            Localization.LoginText.UseLanguage("zh-CN");
        }

        if (arguments.Length >= 1 && arguments[0] == "--switch-region")
        {
            _ = RunRegionSwitchAsync(arguments);
            return;
        }

        window = new LoginWindow(arguments.Contains("--preview-pid") || arguments.Contains("--preview-welcome-pid") ||
                                 arguments.Contains("--preview-global-pid") ||
                                 arguments.Contains("--preview-taiwan-pid"));
        _ = RunAsync(arguments);
    }

    private void Shutdown(int exitCode) => lifetime.Shutdown(exitCode);

    private async Task VerifyUiAsync(string directory)
        => Shutdown(await LoginWindow.VerifyNativeUiAsync(directory));

    private async Task RunRegionSwitchAsync(string[] arguments)
    {
        try
        {
            NteRegionSwitchOptions options = NteRegionSwitchOptions.Parse(arguments);
            Shutdown(await ShowRegionSwitchAsync(options, false) ? 0 : 1);
        }
        catch (Exception error)
        {
            NteHostStartupDiagnostics.TryWrite(NteHostStartupDiagnostics.DefaultPath, error, "ServerSwitchFailed");
            LoginWindow failure = new();
            try
            {
                await failure.ShowFailureAsync(Localization.LoginText.Get("ServerSwitchFailed"));
            }
            finally
            {
                failure.Shutdown();
                Shutdown(1);
            }
        }
    }

    private static async Task<bool> ShowRegionSwitchAsync(NteRegionSwitchOptions options, bool closeOnCompletion)
    {
        Localization.LoginText.UseLanguage(options.Language);
        ViewModels.RegionSwitchViewModel model = new(new Services.NativeRegionSwitchClient(), options,
            key => Localization.LoginText.Get(key));
        RegionSwitchWindow view = new(model);
        TaskCompletionSource closed = new(TaskCreationOptions.RunContinuationsAsynchronously);
        view.Closed += (_, _) => closed.TrySetResult();
        if (closeOnCompletion) model.Completed += () => Dispatcher.UIThread.Post(view.Close);
        view.Show();
        await closed.Task;
        return model.IsCompleted;
    }

    private static bool NeedsServerActivation(NteHostLaunchOptions options)
    {
        if (options.Region is not (NteHostRegion.Global or NteHostRegion.Taiwan)) return false;
        if (System.IO.File.Exists(System.IO.Path.Combine(options.GameRoot, ".collapse-nte", "switch.json")))
            return true;
        string state = System.IO.Path.Combine(options.GameRoot, ".collapse-nte", "state.json");
        if (!System.IO.File.Exists(state)) return false;
        if (new System.IO.FileInfo(state).Length > 16 * 1024 * 1024)
            throw new System.IO.InvalidDataException("Invalid shared installation state.");
        using var document = System.Text.Json.JsonDocument.Parse(System.IO.File.ReadAllBytes(state));
        return document.RootElement.GetProperty("ActiveAppId").GetString() !=
               NteRegionDefinition.From(options.Region).AppId;
    }

    private async Task RunAsync(string[] arguments)
    {
        int exitCode = 0;
        bool officialLaunch = false;
        try
        {
            if (arguments.Length is 2 or 4 && arguments[0] is "--preview-global-pid" or "--preview-taiwan-pid" &&
                int.TryParse(arguments[1], out int globalPid))
            {
                if (arguments[0] == "--preview-taiwan-pid") await window!.PreviewTaiwanAsync(globalPid);
                else await window!.PreviewGlobalAsync(globalPid);
                return;
            }

            // Owned preview keeps the production taskbar/window styles while
            // using the same isolated, non-authenticating fixture accounts.
            if (arguments.Length == 2 && arguments[0] is "--preview-pid" or "--preview-owned-pid" or "--preview-welcome-pid" &&
                int.TryParse(arguments[1], out int pid))
            {
                await window!.PreviewAsync(pid, arguments[0] == "--preview-welcome-pid",
                    arguments[0] == "--preview-owned-pid");
                return;
            }

            NteHostLaunchOptions options = NteHostLaunchOptions.Parse(arguments);
            if (NeedsServerActivation(options))
                throw new InvalidOperationException("Activate the selected installation before using TouchUILaunch.");
            window!.ConfigureLaunch(options);

            if (options.Region is NteHostRegion.Global or NteHostRegion.Taiwan)
            {
                using OverseasGameLoginHost host = new(options, window!);
                if (!await host.RunAsync()) exitCode = 1;
            }
            else
            {
                using GameLoginHost host = new(options, window!);
                if (!await host.RunAsync()) exitCode = 1;
            }
        }
        catch (Exception ex)
        {
            exitCode = 1;
            if (arguments.Length == 2 && arguments[0] is "--preview-pid" or "--preview-owned-pid" or "--preview-welcome-pid")
                System.IO.File.WriteAllText(
                    System.IO.Path.Combine(AppContext.BaseDirectory, "preview-startup-error.txt"), ex.ToString());
            string key = officialLaunch && ex is System.IO.FileNotFoundException
                ? "OfficialLauncherMissing"
                : ex is NteGlobalLaunchException global
                    ? global.Key
                    : ex is NteCnLaunchException china
                        ? china.Key
                        : ex is System.ComponentModel.Win32Exception native && native.NativeErrorCode == 740
                            ? "ElevationRequired"
                            : "StartupFailed";
            NteHostStartupDiagnostics.TryWrite(NteHostStartupDiagnostics.DefaultPath, ex, key);
            await window!.ShowFailureAsync(Localization.LoginText.Get(key));
        }
        finally
        {
            window!.Shutdown();
            Shutdown(exitCode);
        }
    }

    private async Task ForwardGlobalBrowserAsync(string uri)
    {
        bool forwarded = false;
        try
        {
            forwarded = await GlobalBrowserLogin.ForwardAsync(uri);
        }
        catch (Exception)
        {
            /* Never show or log a callback URI. */
        }

        Shutdown(forwarded ? 0 : 1);
    }
}
