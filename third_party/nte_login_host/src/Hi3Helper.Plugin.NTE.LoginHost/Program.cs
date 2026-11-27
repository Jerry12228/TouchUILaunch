using Avalonia;
using Avalonia.Controls;
using Avalonia.Controls.ApplicationLifetimes;

namespace Nte.LoginHost;

internal static class Program
{
    [STAThread]
    private static int Main(string[] args)
    {
        if (args is ["--verify-integration"]) return IntegrationVerification.Run();
        if (args is ["--verify-installation", var root, var region]) return IntegrationVerification.VerifyInstallation(root, region);
        if (args is ["--verify-owned-child", var executable, var folder, var report])
            return IntegrationVerification.OwnedChild(executable, folder, report);
        // Browser callbacks only forward to the already running host. They do
        // not need to load a renderer, fonts, or create a second login window.
        if (args.Length == 2 && args[0] == "--global-web-callback")
        {
            try
            {
                return GlobalBrowserLogin.ForwardAsync(args[1]).GetAwaiter().GetResult() ? 0 : 1;
            }
            catch (Exception)
            {
                return 1;
            } // Never log a callback URI.
        }

        if (args.Length == 1 && args[0] == "--verify-resources")
            return Localization.LoginText.VerifyCatalogs();

        return AppBuilder.Configure<App>().UseWin32().UseSkia()
            .StartWithClassicDesktopLifetime(args, ShutdownMode.OnExplicitShutdown);
    }
}
