using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;
using Nte.Login;
using Nte.LoginHost.Localization;

namespace Nte.LoginHost;

internal sealed partial class LoginWindow
{
    private OverseasLoginView? globalView;

    internal async Task<bool> RequestGlobalAsync(NteGlobalAuthentication api,
        Func<NteGlobalIdentity, CancellationToken, Task> submit, CancellationToken token,
        OverseasAccountStore? accounts = null, bool previewMode = false, string? notice = null)
    {
        CancelFlow();
        await activeWork;
        Services.OverseasLoginService service = new(api, accounts, handle, submit,
            (mayPrompt, action) => WithGlobalAccountOwnerAsync(mayPrompt, _ => action()));
        using OverseasLoginView content = new(service, FindStyle, previewMode, notice);
        globalView = content;
        Title = content.TitleLabel;
        content.LayoutChanged += () =>
        {
            QueueCardSize();
            QueueBodyWidth();
            Track();
        };
        requested = true;
        footer.IsVisible = false;
        login.IsDefault = false;
        ResetBackdrop();
        form.Children.Clear();
        Add(form, content, 0);
        title.Text = content.TitleLabel;
        subtitle.IsVisible = modeRail.IsVisible = status.IsVisible = login.IsVisible = false;
        UpdateCardSize();
        QueueBodyWidth();
        Track(raise: true);
        try
        {
            return await content.RunAsync(token);
        }
        finally
        {
            globalView = null;
            requested = false;
            HidePresented();
            ResetBackdrop();
            Track();
            login.IsVisible = true;
            footer.IsVisible = true;
        }
    }

    internal async Task WithGlobalAccountOwnerAsync(bool mayPrompt, Func<nint, Task> action)
    {
        if (!mayPrompt)
        {
            await action(handle);
            return;
        }

        bool transient = !requested;
        bool previousLogin = login.IsVisible;
        bool previousFooter = footer.IsVisible;
        if (transient)
        {
            requested = true;
            Title = LoginText.Get(launchOptions?.Region == NteHostRegion.Taiwan ? "TaiwanTitle" : "GlobalTitle");
            footer.IsVisible = false;
            title.Text = Title;
            form.Children.Clear();
            Add(form, new Avalonia.Controls.TextBlock
            {
                Text = LoginText.Get("UnlockAccountWithHello"), FontSize = 13, TextWrapping = TextWrapping.Wrap
            }, 0);
            subtitle.IsVisible = modeRail.IsVisible = status.IsVisible = login.IsVisible = false;
            UpdateCardSize();
            QueueBodyWidth();
        }

        try
        {
            await WithHelloOwnerAsync(true, async () =>
            {
                await action(handle);
                return true;
            });
        }
        finally
        {
            if (transient)
            {
                requested = false;
                login.IsVisible = previousLogin;
                footer.IsVisible = previousFooter;
                HidePresented();
                ResetBackdrop();
                Track();
            }
        }
    }

    internal Task PreviewGlobalAsync(int pid) => PreviewOverseasAsync(pid, NteOverseasRegion.Global);
    internal Task PreviewTaiwanAsync(int pid) => PreviewOverseasAsync(pid, NteOverseasRegion.Taiwan);

    private async Task PreviewOverseasAsync(int pid, NteOverseasRegion region)
    {
        Attach(pid);
        preview = true;
        using NteOverseasApplicationConfiguration config = new(new byte[40], region.PackageName,
            region.Service, region.Area, region: region);
        using NteGlobalAuthentication api = new(config, "preview");
        using CancellationTokenSource lifetime = new();
        Task formTask = RequestGlobalAsync(api, (_, _) => Task.CompletedTask, lifetime.Token, previewMode: true);
        await game!.WaitForExitAsync();
        lifetime.Cancel();
        await formTask;
    }
}
