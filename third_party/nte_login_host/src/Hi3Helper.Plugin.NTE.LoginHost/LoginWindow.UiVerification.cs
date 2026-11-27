using System.Text.Json;
using Avalonia;
using Avalonia.Automation;
using Avalonia.Controls;
using Avalonia.Media.Imaging;
using Avalonia.VisualTree;
using Nte.Login;
using Nte.LoginHost.Accounts;
using Nte.LoginHost.Localization;
using Nte.LoginHost.Services;
using Nte.LoginHost.ViewModels;

namespace Nte.LoginHost;

internal sealed partial class LoginWindow
{
    // An opt-in, isolated test entry point in the actual executable. The old
    // verifier loaded a WPF DLL with reflection and could not exercise AOT.
    // Every service below fails if asked to access accounts or the network.
    internal static async Task<int> VerifyNativeUiAsync(string directory)
    {
        directory = Path.GetFullPath(directory);
        Directory.CreateDirectory(directory);
        List<string> checks = [];
        string currentCase = "startup";
        try
        {
            foreach (string language in new[] { "zh-CN", "zh-TW", "en", "de", "fr", "es", "ru", "ja", "ko" })
            foreach (NteHostRegion region in new[] { NteHostRegion.CN, NteHostRegion.Global, NteHostRegion.Taiwan })
            {
                currentCase = region + "/" + language;
                LoginText.UseLanguage(language);
                LoginWindow window = new(true)
                {
                    Width = 1180, Height = 730, Position = new(-20000, -20000),
                    preview = true, initialized = true, requested = true, syntheticLayoutOnly = true
                };
                try
                {
                    window.view.Width = SceneWidth;
                    if (region == NteHostRegion.CN) window.SetMainlandPreviewAccounts(3);
                    else
                    {
                        window.globalView = new(new VerificationLoginService(NteOverseasRegion.From(region)),
                            window.FindStyle, true);
                        window.form.Children.Clear();
                        Add(window.form, window.globalView, 0);
                        window.footer.IsVisible = false;
                        window.title.Text = window.globalView.TitleLabel;
                        window.subtitle.IsVisible = false;
                        window.globalView.LayoutChanged += window.QueueCardSize;
                    }

                    window.Show();
                    await Layout();
                    Check(Visible("account-selector") && Visible("submit"), "saved-page");
                    Check(!Visible("remember") && !Visible("agreement") && !Visible("provider-google"),
                        "saved-page-only-restores");
                    Check(Node<ListBox>("account-selector").ItemCount == 3, "three-synthetic-accounts");
                    Check(window.scroll.Extent.Height <= window.scroll.Viewport.Height + 1, "saved-page-fits");
                    foreach (string theme in new[] { "day", "night", "sunset" })
                    {
                        window.ApplyTheme(theme);
                        await Layout();
                        if (language == "zh-CN") await Capture("saved-" + theme);
                    }

                    await Click("manage-accounts");
                    Check(Visible("remark") && !Visible("submit"), "manage-is-separate");
                    Node<TextBox>("remark").Text = "Native AOT fixture";
                    await Click("save-remark");
                    Check(Visible("account-selector") && !Visible("remark"), "rename-returns-to-list");
                    await Click("manage-accounts");
                    await Click("remove-account");
                    Check(Visible("confirm-remove") && !Visible("remark"), "remove-requires-confirmation");
                    await Click("cancel-remove");
                    await Click("cancel-edit");
                    Check(Node<ListBox>("account-selector").ItemCount == 3, "cancel-preserves-accounts");
                    await Click("other-account");
                    if (region != NteHostRegion.CN)
                    {
                        Check(Visible("provider-google") && Visible("provider-facebook"), "provider-methods");
                        Check(Visible("email-login") == (region == NteHostRegion.Global), "region-email-availability");
                        Check(Visible("phone-login") == (region == NteHostRegion.Taiwan), "region-phone-availability");
                        await Click(region == NteHostRegion.Global ? "email-login" : "phone-login");
                        Check(Visible("agreement") && Visible("remember"), "new-login-consent");
                        string prefix = region == NteHostRegion.Global ? "email" : "phone";
                        await Click(prefix + "-password-mode");
                        TextBox secret = Node<TextBox>("secret");
                        Check(secret.PasswordChar != '\0' && !secret.IsUndoEnabled, "password-is-masked-without-undo");
                        secret.Text = "synthetic-only";
                        await Click(prefix + "-code-mode");
                        Check(string.IsNullOrEmpty(secret.Text), "switching-method-clears-password");
                        Check(Visible(prefix + "-code") && !Visible("secret"), "code-mode-layout");
                        if (region == NteHostRegion.Taiwan)
                        {
                            ComboBox countries = Node<ComboBox>("phone-country");
                            Check(countries.ItemCount > 0 && countries.SelectedItem is NtePhoneCountry,
                                "typed-country-selector");
                            countries.IsDropDownOpen = true;
                            await Layout();
                            Check(countries.IsDropDownOpen, "country-popup-opens");
                            countries.IsDropDownOpen = false;
                        }
                    }
                    else
                    {
                        Check(Visible("phone") && Visible("secret") && Visible("remember"), "mainland-sms-layout");
                        window.SetAccountControlsEnabled(true);
                        await window.SendSmsFromUiAsync();
                        Check(!window.sms.IsEnabled && window.smsRemainingSeconds > 0, "sms-cooldown");
                        await window.SwitchModeAsync(true);
                        await window.SwitchModeAsync(false);
                        Check(!window.sms.IsEnabled && window.smsRemainingSeconds > 0, "cooldown-survives-page-switch");
                    }

                    await Layout();
                    Check(window.scroll.Extent.Height <= window.scroll.Viewport.Height + 1, "new-login-fits");
                    if (language == "zh-CN") await Capture("new-login");
                    window.noticeGlobal = region != NteHostRegion.CN;
                    window.ShowLoginNotice();
                    await Layout();
                    Check(window.helpOpen && window.scroll.Extent.Height > window.scroll.Viewport.Height,
                        "notice-scrolls");
                    Check(Visible("official-launcher") && Visible("submit"), "notice-actions-stay-visible");
                    window.ChooseOfficialLauncher();
                    await Layout();
                    Check(!window.content.IsEnabled && Visible("confirm-official-launcher"),
                        "official-choice-is-modal");
                    await Click("cancel-official-launcher");
                    Check(window.content.IsEnabled && window.confirmationOverlay == null, "cancel-restores-notice");
                    if (language == "zh-CN") await Capture("notice");

                    void Check(bool passed, string name)
                    {
                        if (!passed) throw new InvalidDataException(currentCase + "/" + name);
                        checks.Add(currentCase + "/" + name);
                    }

                    bool Displayed(Control control) => control.IsVisible &&
                                                       control.GetVisualAncestors().OfType<Control>()
                                                           .All(parent => parent.IsVisible);

                    IEnumerable<Control> Nodes() => window.GetVisualDescendants().OfType<Control>();

                    bool Visible(string id) => Nodes().Any(control =>
                        AutomationProperties.GetAutomationId(control) == id && Displayed(control));

                    T Node<T>(string id) where T : Control => Nodes().OfType<T>().Single(control =>
                        AutomationProperties.GetAutomationId(control) == id && Displayed(control));

                    async Task Click(string id)
                    {
                        Button button = Node<Button>(id);
                        Check(button.IsEnabled, id + "-enabled");
                        if (button is RadioButton radio) radio.IsChecked = true;
                        else if (button.Command is AsyncRelayCommand asyncCommand) await asyncCommand.ExecuteAsync();
                        else if (button.Command is { } command) command.Execute(button.CommandParameter);
                        else button.RaiseEvent(new RoutedEventArgs(Button.ClickEvent));
                        await Layout();
                    }

                    async Task Layout()
                    {
                        await Dispatcher.UIThread.InvokeAsync(() => { }, DispatcherPriority.Background);
                        window.UpdateCardSize();
                        window.view.Height = window.scene.Height;
                        window.UpdateLayout();
                        await Dispatcher.UIThread.InvokeAsync(() => { }, DispatcherPriority.Background);
                        window.UpdateCardSize();
                        window.view.Height = window.scene.Height;
                        window.UpdateLayout();
                    }

                    async Task Capture(string name)
                    {
                        await Task.Delay(220); // Finish the selector transition before exporting.
                        using RenderTargetBitmap bitmap =
                            new(new PixelSize(912, (int)Math.Ceiling(window.scene.Height * 2)), new Vector(192, 192));
                        bitmap.Render(window.scene);
                        bitmap.Save(Path.Combine(directory, region + "-" + name + ".png"));
                    }
                }
                finally
                {
                    window.globalView?.Dispose();
                    window.Shutdown();
                }
            }

            // Round-trip synthetic bytes through the deployed source-generated
            // account serializers and real current-user protection, without a
            // Hello prompt or access to the user's existing account directory.
            currentCase = "synthetic-account-serialization";
            string accountDirectory = Path.Combine(directory, "fixture-accounts-" + Guid.NewGuid().ToString("N"));
            WindowsAccountKeyProtector protector = new();
            LocalAccountStore accountStore = new(accountDirectory, protector);
            AccountDisplayStore displayStore = new(accountDirectory, protector);
            byte[] fixtureCredential = "Native AOT synthetic credential"u8.ToArray();
            LocalAccount account =
                await accountStore.SaveAsync("AOT fixture", fixtureCredential, 0, CancellationToken.None);
            if (accountStore.List().Single().Id != account.Id ||
                !(await accountStore.UnlockAsync(account.Id, 0, CancellationToken.None)).SequenceEqual(
                    fixtureCredential))
                throw new InvalidDataException(currentCase);
            LocalAccount renamed = await accountStore.RenameAsync(account.Id, "AOT renamed", 0, CancellationToken.None);
            using (PendingAccountSave pending = await displayStore.AttachAsync(new(renamed, () => { }, () => { }),
                       new(8, Email: "native-aot@example.invalid", InitialLabel: renamed.Label),
                       CancellationToken.None))
                pending.Commit();
            if ((await displayStore.ReadAsync(renamed, CancellationToken.None))?.Email != "native-aot@example.invalid")
                throw new InvalidDataException("synthetic-display-serialization");
            displayStore.Remove(account.Id);
            accountStore.Remove(account.Id);
            if (accountStore.List().Count != 0) throw new InvalidDataException("synthetic-account-removal");
            checks.Add("native-account-and-display-serialization");

            // Compiled bindings must remain live after AOT and trimming.
            LoginText.UseLanguage("zh-CN");
            RegionSwitchViewModel switchModel = new(new VerificationSwitchService(),
                new(directory, NteHostRegion.Taiwan), key => LoginText.Get(key));
            RegionSwitchWindow switchWindow = new(switchModel) { Position = new(-20000, -20000) };
            try
            {
                switchWindow.Show();
                await Dispatcher.UIThread.InvokeAsync(() => { }, DispatcherPriority.Background);
                await switchModel.CheckCommand.ExecuteAsync();
                await switchModel.SwitchCommand.ExecuteAsync();
                await Dispatcher.UIThread.InvokeAsync(() => { }, DispatcherPriority.Background);
                if (!switchModel.IsCompleted || !switchWindow.GetVisualDescendants().OfType<ProgressBar>()
                        .Any(bar => bar.Value == 100))
                    throw new InvalidDataException("compiled-region-switch-bindings");
                checks.Add("compiled-region-switch-bindings");
            }
            finally
            {
                switchWindow.Close();
            }

            Report(true, null);
            return 0;
        }
        catch (Exception error)
        {
            Report(false, currentCase + ": " + error.GetType().Name + ": " + error.Message);
            return 1;
        }

        void Report(bool passed, string? error) => File.WriteAllText(Path.Combine(directory, "native-ui-report.json"),
            JsonSerializer.Serialize(new Dictionary<string, object?>
            {
                ["passed"] = passed, ["checks"] = checks.ToArray(), ["count"] = checks.Count,
                ["error"] = error, ["renderer"] = "Avalonia",
                ["dynamicCodeSupported"] = System.Runtime.CompilerServices.RuntimeFeature.IsDynamicCodeSupported,
                ["syntheticOnly"] = true, ["accountRequests"] = 0
            }, NteJsonContext.Default.DictionaryStringObject));
    }

    private sealed class VerificationLoginService(NteOverseasRegion region) : IOverseasLoginService
    {
        public NteOverseasRegion Region => region;
        public bool Initialized => true;
        public TimeSpan CaptchaCooldown => TimeSpan.Zero;
        public IReadOnlyList<LocalAccount> ListAccounts() => throw Unexpected();
        public Task InitializeAsync(CancellationToken token) => throw Unexpected();
        public Task<GlobalAccountSnapshot> LoadAccountsAsync(CancellationToken token) => throw Unexpected();
        public bool RequiresHello(Guid id) => throw Unexpected();
        public Task RestoreAsync(Guid id, CancellationToken token) => throw Unexpected();
        public Task RenameAsync(Guid id, string label, CancellationToken token) => throw Unexpected();
        public Task RemoveAsync(Guid id, CancellationToken token) => throw Unexpected();
        public Task SendCodeAsync(string email, CancellationToken token) => throw Unexpected();

        public Task SignInEmailAsync(string email, string secret, bool password, bool remember,
            CancellationToken token) => throw Unexpected();

        public Task SignInBrowserAsync(int provider, bool remember, CancellationToken token) => throw Unexpected();

        private static Exception Unexpected() =>
            new InvalidOperationException("Synthetic UI verification cannot access a login service.");
    }

    private sealed class VerificationSwitchService : INteRegionSwitchClient
    {
        public Task<NteSwitchSummary> PlanAsync(NteRegionSwitchOptions options, IProgress<NteSwitchProgress> progress,
            CancellationToken token)
            => Task.FromResult(
                new NteSwitchSummary("fixture", options.TargetAppId, "fixture", 3, 0, 0, 0, false, false));

        public Task<NteSwitchSummary> SwitchAsync(NteRegionSwitchOptions options, IProgress<NteSwitchProgress> progress,
            CancellationToken token)
            => Task.FromResult(new NteSwitchSummary("fixture", options.TargetAppId, "fixture", 3, 0, 0, 0, true,
                false));
    }
}
