using System.ComponentModel;
using System.IO;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;
using Avalonia.Automation;

using Nte.Login;
using Nte.LoginHost.Localization;

namespace Nte.LoginHost;

internal sealed partial class LoginWindow
{
    private NteHostLaunchOptions? launchOptions;
    private TaskCompletionSource<bool>? noticeCompletion;
    private readonly NteLoginNotice notice = new(NteLoginNotice.DefaultPath);
    private readonly Button officialLauncher = new();
    private bool noticeGlobal, noticeConfirmation;
    private Border? confirmationOverlay;
    private string? noticeError;
    private bool MandatoryNotice => noticeCompletion != null;

    internal void ConfigureLaunch(NteHostLaunchOptions options) => launchOptions = options;

    internal async Task<bool> EnsureNoticeAsync(bool global, CancellationToken token)
    {
        if (notice.IsAcknowledged()) return true;
        noticeCompletion = new(TaskCreationOptions.RunContinuationsAsynchronously);
        Task<bool> answer = noticeCompletion.Task;
        requested = true;
        automaticResumeInProgress = false;
        noticeGlobal = global;
        ShowLoginNotice();
        Track(raise: true);
        using CancellationTokenRegistration registration = token.Register(() => Dispatcher.UIThread.Post(CancelForGameExit));
        try { return await answer; }
        finally
        {
            RemoveNoticeConfirmation();
            noticeCompletion = null;
            helpOpen = false;
            officialLauncher.IsVisible = false;
            closeLogin.IsVisible = true;
            requested = false;
            HidePresented();
        }
    }

    private void ShowLoginNotice()
    {
        RemoveNoticeConfirmation();
        helpOpen = true;
        noticeConfirmation = false;
        form.Children.Clear();
        title.Text = LoginText.Get("SignInHelp");
        subtitle.IsVisible = modeRail.IsVisible = status.IsVisible = false;
        footer.IsVisible = login.IsVisible = true;
        closeLogin.IsVisible = !(MandatoryNotice);
        otherMainlandAccount.IsVisible = false;
        login.Content = LoginText.Get("NoticeUnderstood");
        login.IsEnabled = true;
        login.IsDefault = true;
        officialLauncher.Content = LoginText.Get("UseOfficialLauncher");
        officialLauncher.IsVisible = false; // Independent host; official relay is selected in the native CLI.
        foreach (string section in new[] { "Purpose", "Risk", "Sharing", "Data", "Storage", "Network", "Retention", "Choice" })
        {
            Add(form, new TextBlock { Text = LoginText.Get("Notice" + section + "Title"), FontSize = 14, FontWeight = FontWeight.SemiBold }, 6);
            string body = section == "Sharing"
                ? LoginText.Get("NoticeSharing", LoginText.Get(noticeGlobal ? "NoticeSharingExamplesGlobal" : "NoticeSharingExamplesMainland"))
                : LoginText.Get("Notice" + section);
            Add(form, new TextBlock { Text = body, FontSize = 12, TextWrapping = TextWrapping.Wrap });
            if (section == "Storage" && (launchOptions?.Region == NteHostRegion.Taiwan || globalView?.Region == NteHostRegion.Taiwan))
                Add(form, new TextBlock { Text = LoginText.Get("NoticeTaiwanData"), FontSize = 12, TextWrapping = TextWrapping.Wrap });
            if (section == "Network" && !noticeGlobal)
                Add(form, new TextBlock { Text = LoginText.Get("NoticeRealName"), FontSize = 12, TextWrapping = TextWrapping.Wrap });
        }
        if (noticeError != null)
            Add(form, new TextBlock { Text = noticeError, FontSize = 12, TextWrapping = TextWrapping.Wrap });
        QueueBodyWidth();
        UpdateCardSize();
        scroll.ScrollToHome();
    }

    private void AcknowledgeNotice()
    {
        if (noticeConfirmation) { CompleteOfficialChoice(); return; }
        if (MandatoryNotice)
        {
            try { if (!preview) notice.Acknowledge(); }
            catch (Exception error) when (error is IOException or UnauthorizedAccessException)
            {
                noticeError = LoginText.Get("NoticeSaveFailed");
                ShowLoginNotice();
                return;
            }
            noticeCompletion!.TrySetResult(true);
            if (preview) { noticeCompletion = null; ReturnFromNotice(); }
        }
        else ReturnFromNotice();
    }

    private void ReturnFromNotice()
    {
        RemoveNoticeConfirmation();
        noticeError = null;
        noticeConfirmation = false;
        helpOpen = false;
        officialLauncher.IsVisible = false;
        closeLogin.IsVisible = true;
        if (globalView is { } global)
        {
            form.Children.Clear();
            Add(form, global, 0);
            title.Text = global.TitleLabel;
            footer.IsVisible = false;
            QueueCardSize();
        }
        else ShowForm();
    }

    private void ChooseOfficialLauncher()
    {
        if (noticeConfirmation) { ShowLoginNotice(); return; }
        noticeConfirmation = true;
        StackPanel dialog = new();
        Add(dialog, new TextBlock { Text = LoginText.Get("UseOfficialLauncher"), FontSize = 17, FontWeight = FontWeight.SemiBold });
        Add(dialog, new TextBlock { Text = LoginText.Get("OfficialLauncherConfirm"), FontSize = 13, TextWrapping = TextWrapping.Wrap });
        Button confirm = new() { Content = LoginText.Get("ConfirmOfficialLauncher"), MinHeight = 42, Theme = FindStyle("AccentButtonStyle") };
        Button cancel = new() { Content = LoginText.Get("CancelOperation"), MinHeight = 36, Margin = new(0, 8, 0, 0), Theme = FindStyle("SmsButtonStyle") };
        AutomationProperties.SetAutomationId(confirm, "confirm-official-launcher");
        AutomationProperties.SetAutomationId(cancel, "cancel-official-launcher");
        confirm.Click += (_, _) => CompleteOfficialChoice();
        cancel.Click += (_, _) => ShowLoginNotice();
        dialog.Children.Add(confirm);
        dialog.Children.Add(cancel);
        Border surface = new() { Child = dialog, Padding = new(20), Margin = new(12), CornerRadius = new(12), VerticalAlignment = VerticalAlignment.Center };
        // Use an opaque themed surface: a modal confirmation must be legible
        // independently of the scene and must intercept every underlying click.
        surface.SetResourceReference(Border.BackgroundProperty, "GlobalPopupBrush");
        confirmationOverlay = new() { Child = surface, Background = new Avalonia.Media.SolidColorBrush(Avalonia.Media.Color.FromArgb(180, 0, 0, 0)), CornerRadius = new(16) };
        Avalonia.Input.KeyboardNavigation.SetTabNavigation(confirmationOverlay, Avalonia.Input.KeyboardNavigationMode.Cycle);
        content.IsEnabled = false;
        ((Grid)card.Child!).Children.Add(confirmationOverlay);
        closeLogin.IsVisible = false;
        cancel.Focus();
    }

    private void RemoveNoticeConfirmation()
    {
        if (confirmationOverlay != null) ((Grid)card.Child!).Children.Remove(confirmationOverlay);
        confirmationOverlay = null;
        content.IsEnabled = true;
        noticeConfirmation = false;
    }

    private void CompleteOfficialChoice()
    {
        try
        {
            if (preview || launchOptions == null)
            {
                noticeError = LoginText.Get("OfficialPreviewOnly");
                ShowLoginNotice();
                return;
            }
            _ = NteOfficialLaunchPlan.Create(launchOptions.GameRoot, launchOptions.Region);
            // Persist first. A failed write leaves this owned game running and
            // the notice unacknowledged. Never terminate another installation.
            NteLaunchPreferences.Update(NteLaunchPreferences.GetPath(launchOptions.Region), officialLauncher: true);
            if (game is { HasExited: false }) game.Kill();
            CancelForGameExit();
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or Win32Exception or InvalidOperationException)
        {
            noticeError = LoginText.Get(error is FileNotFoundException ? "OfficialLauncherMissing" : "OfficialSwitchFailed");
            ShowLoginNotice();
        }
    }
}
