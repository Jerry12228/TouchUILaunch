using System.IO;
using System.Security.Cryptography;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;
using Avalonia.Automation;

using Nte.Login;
using Nte.LoginHost.Accounts;
using Nte.LoginHost.Localization;

namespace Nte.LoginHost;

internal sealed partial class LoginWindow
{
    private static string MainlandAccountDirectory => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "TouchUILaunch", "NTE", "Accounts");

    private readonly LocalAccountStore accounts = new(MainlandAccountDirectory, new WindowsAccountKeyProtector(),
        helloProtector: new HelloAccountKeyProtector());

    private readonly RememberedLogin remembered = new(Path.Combine(MainlandAccountDirectory, "last-login.bin"),
        new WindowsAccountKeyProtector());

    private readonly AccountDisplayStore mainlandDisplays =
        new(MainlandAccountDirectory, new WindowsAccountKeyProtector());

    private readonly Dictionary<Guid, AccountDisplayProfile> mainlandProfiles = [];
    private SavedAccountPicker? mainlandPicker;
    private readonly Button otherMainlandAccount = new();
    private readonly Border modeRail = new() { IsVisible = false };
    private readonly CheckBox saveAccount = new() { MinHeight = 28, FontSize = 12, IsChecked = true };
    private Task rememberedChange = Task.CompletedTask;
    private bool automaticAttempt, autoResumeDisabled;
    private bool localMode, initialized, editingLabel, experimentalHello;

    private readonly List<LocalAccount> previewAccounts =
    [
        new(new Guid("00000000-0000-0000-0000-000000000001"), LoginText.Get("PreviewPrimary"), DateTimeOffset.UtcNow,
            null),
        new(new Guid("00000000-0000-0000-0000-000000000002"), LoginText.Get("PreviewSecondary"), DateTimeOffset.UtcNow,
            null),
        new(new Guid("00000000-0000-0000-0000-000000000003"), LoginText.Get("PreviewSpare"), DateTimeOffset.UtcNow,
            null)
    ];

    private Task activeWork = Task.CompletedTask;
    private CancellationToken requestToken;
    private Func<CancellationToken, Task>? resetAdmission;
    private LocalAccount? selectedAccount;
    private int helloPrompts;

    private async Task<T> WithHelloOwnerAsync<T>(bool mayPrompt, Func<Task<T>> action)
    {
        if (!mayPrompt) return await action();
        helloPrompts++;
        automaticResumeInProgress = false;
        welcome?.Dismiss();
        Track();
        try
        {
            using HelloPromptWindowScope presentation = new(handle);
            return await action();
        }
        finally
        {
            helloPrompts--;
            Track();
        }
    }

    private void ConfigureAccounts()
    {
        saveAccount.Theme = FindStyle("FormCheckBoxStyle");
        saveAccount.Content = new TextBlock { Text = LoginText.Get("SaveOnDevice"), TextWrapping = TextWrapping.Wrap };
        AutomationProperties.SetName(saveAccount, LoginText.Get("SaveOnDevice"));
        AutomationProperties.SetAutomationId(saveAccount, "remember");
        AutomationProperties.SetName(phone, LoginText.Get("Phone"));
        AutomationProperties.SetAutomationId(phone, "phone");
        AutomationProperties.SetName(captcha, LoginText.Get("SmsCode"));
        AutomationProperties.SetAutomationId(captcha, "secret");
        AutomationProperties.SetAutomationId(sms, "send-code");
        AutomationProperties.SetAutomationId(login, "submit");
        AutomationProperties.SetAutomationId(status, "status");
        mainlandPicker = new(FindStyle, PresentMainland);
        mainlandPicker.SelectionChanged += async () =>
        {
            if (mainlandPicker.Selected is not { } selected || selectedAccount?.Id == selected.Id) return;
            await SwitchModeAsync(true, selected);
        };
        mainlandPicker.LayoutChanged += () =>
        {
            RefreshMainlandNavigation();
            UpdateCardSize();
        };
        mainlandPicker.RenameRequested += RenameMainlandAsync;
        mainlandPicker.RemoveRequested += async () =>
        {
            if (selectedAccount is { } selected) await RemoveAccountAsync(selected);
        };
        otherMainlandAccount.Theme = FindStyle("QuietButtonStyle");
        otherMainlandAccount.MinHeight = 28;
        otherMainlandAccount.Margin = new(0, 6, 0, 0);
        otherMainlandAccount.FontSize = 12;
        AutomationProperties.SetAutomationId(otherMainlandAccount, "other-account");
        otherMainlandAccount.Click += async (_, _) => await SwitchModeAsync(!localMode);
        footer.Children.Add(otherMainlandAccount);
    }

    private void AddAccountTabs() => modeRail.IsVisible = false;
    private void AddAccountSaveFields() => Add(form, saveAccount, 0);

    private IReadOnlyList<LocalAccount> MainlandAccounts() =>
        preview ? previewAccounts : requested ? accounts.List() : [];

    private SavedAccountChoice PresentMainland(LocalAccount account)
    {
        mainlandProfiles.TryGetValue(account.Id, out AccountDisplayProfile? profile);
        string primary = profile?.InitialLabel == account.Label && profile.MaskedPhone != null
            ? profile.MaskedPhone
            : account.Label;
        string secondary = profile?.MaskedPhone != null ? primary == profile.MaskedPhone ? "" : profile.MaskedPhone
            : System.Text.RegularExpressions.Regex.IsMatch(account.Label, @"\A1[0-9]{2}\*{4}[0-9]{4}\z") ? ""
            : LoginText.Get("SavedAccountNumber", mainlandPicker!.Reference(account.Id));
        return new(account, primary, secondary, "SMS");
    }

    private async Task LoadMainlandDisplaysAsync(CancellationToken token)
    {
        foreach (LocalAccount account in accounts.List())
        {
            try
            {
                if (await mainlandDisplays.ReadAsync(account, token) is { } profile)
                    mainlandProfiles[account.Id] = profile;
            }
            catch (Exception error) when (AccountDisplayStore.IsDisplayError(error))
            {
                status.Text = LoginText.Get("DisplayInfoUnavailable");
            }
        }
    }

    private void BuildAccounts()
    {
        IReadOnlyList<LocalAccount> entries = MainlandAccounts();
        if (entries.Count == 0)
        {
            selectedAccount = null;
            localMode = false;
            ShowForm();
            return;
        }

        selectedAccount = entries.FirstOrDefault(account => account.Id == selectedAccount?.Id) ?? entries[0];
        mainlandPicker!.Reload(entries, selectedAccount.Id);
        Add(form, mainlandPicker, 0);
        login.Content = !initialized && !preview ? LoginText.Get("RetryConnection") : LoginText.Get("ContinueSignIn");
        login.IsEnabled = !busy && (initialized || preview || requested);
        RefreshMainlandNavigation();
    }

    private void RefreshMainlandNavigation()
    {
        editingLabel = mainlandPicker?.IsManaging == true && localMode;
        bool hasAccounts = MainlandAccounts().Count > 0;
        otherMainlandAccount.IsVisible = !helpOpen && !editingLabel && (localMode || hasAccounts);
        otherMainlandAccount.Content = new TextBlock
        {
            Text = LoginText.Get(localMode ? "UseOtherAccount" : "BackToSavedAccounts"),
            TextWrapping = TextWrapping.Wrap, TextAlignment = TextAlignment.Center
        };
        login.IsVisible = !(editingLabel);
        login.IsDefault = !editingLabel;
    }

    private Task RenameMainlandAsync()
    {
        if (selectedAccount is not { } selected || mainlandPicker == null) return Task.CompletedTask;
        string value = mainlandPicker.Remark;
        if (value.Length is < 1 or > 32 || value.Any(char.IsControl))
        {
            status.Text = LoginText.Get("InvalidLabel");
            return Task.CompletedTask;
        }

        if (preview)
        {
            int index = previewAccounts.FindIndex(account => account.Id == selected.Id);
            previewAccounts[index] = selected with { Label = value };
            selectedAccount = previewAccounts[index];
            mainlandPicker.EndManage();
            ShowForm();
            return Task.CompletedTask;
        }

        return ExecuteAsync(async (_, token, generation) =>
        {
            selectedAccount = await WithHelloOwnerAsync(accounts.RequiresHello(selected.Id),
                () => accounts.RenameAsync(selected.Id, value, handle, token));
            RequireCurrent(generation, token);
            mainlandPicker.EndManage();
            ShowForm();
            status.Text = LoginText.Get("LabelUpdated");
        });
    }

    private async Task RemoveAccountAsync(LocalAccount account)
    {
        if (!await SwitchModeAsync(true)) return;
        try
        {
            if (preview) previewAccounts.Remove(account);
            else
            {
                mainlandDisplays.Remove(account.Id);
                accounts.Remove(account.Id);
            }
        }
        catch (Exception)
        {
            status.Text = LoginText.Get("RemoveAccountFailed");
            return;
        }

        if (selectedAccount?.Id == account.Id) selectedAccount = null;
        mainlandPicker?.EndManage();
        if (MainlandAccounts().Count == 0) localMode = false;
        ShowForm();
        status.Text = LoginText.Get("AccountRemoved");
        if (!preview)
        {
            try
            {
                await rememberedChange;
                if (await remembered.ReadAsync(CancellationToken.None) == account.Id)
                    await remembered.SetAsync(null, CancellationToken.None);
            }
            catch (Exception)
            {
                autoResumeDisabled = true;
                status.Text = LoginText.Get("AccountRemovedStateRetained");
            }
        }

        if (preview) WritePreviewState();
    }

    private void SetAccountControlsEnabled(bool enabled)
    {
        accountControlsEnabled = enabled;
        phone.IsEnabled = captcha.IsEnabled = saveAccount.IsEnabled = otherMainlandAccount.IsEnabled = enabled;
        if (mainlandPicker != null) mainlandPicker.IsEnabled = enabled;
        UpdateSmsButton();
        login.IsEnabled = helpOpen || enabled || (!busy && requested && !initialized);
    }

    private async Task<bool> SwitchModeAsync(bool local, LocalAccount? selection = null)
    {
        bool interrupted = busy || !activeWork.IsCompleted;
        int generation = ++attempt;
        operation?.Cancel();
        busy = true;
        SetAccountControlsEnabled(false);
        if (!activeWork.IsCompleted) status.Text = LoginText.Get("CancellingPrevious");
        await activeWork;
        if (generation != attempt || !requested) return false;
        operation?.Dispose();
        operation = CancellationTokenSource.CreateLinkedTokenSource(requestToken);
        try
        {
            if (resetAdmission != null) await resetAdmission(operation.Token);
        }
        catch (OperationCanceledException)
        {
            return false;
        }

        if (generation != attempt) return false;
        busy = false;
        if (interrupted) captcha.Clear();
        localMode = local;
        if (selection != null) selectedAccount = selection;
        mainlandPicker?.EndManage();
        status.Text = "";
        SetAccountControlsEnabled(initialized || preview);
        ShowForm();
        return true;
    }

    internal void SetMainlandPreviewAccounts(int count)
    {
        if (!preview || count is < 0 or > 16) throw new InvalidOperationException("Synthetic accounts only.");
        previewAccounts.Clear();
        mainlandProfiles.Clear();
        for (int i = 0; i < count; i++)
        {
            Guid id = new(i + 1, 0, 0, new byte[8]);
            string label = "Fixture " + (i + 1);
            previewAccounts.Add(new(id, label, DateTimeOffset.UtcNow, null));
            mainlandProfiles[id] = new(0, MaskedPhone: $"138****{i + 1:0000}", InitialLabel: label);
        }

        selectedAccount = null;
        localMode = count > 0;
        ShowForm();
    }

    private void RequireCurrent(int generation, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        if (generation != attempt || !requested) throw new OperationCanceledException(token);
    }

    private async Task LoginWithSelectedAccountAsync(ProbeAuthenticationClient api, CancellationToken token,
        int generation)
    {
        bool useLocal = localMode, save = !localMode && saveAccount.IsChecked == true;
        experimentalHello = ReadProtectionPreference();
        string label = useLocal ? selectedAccount?.Label ?? "" : AccountLabels.FromPhone((phone.Text ?? "").Trim());
        ProbeIdentityEnvelope? channel = null, identity = null;
        try
        {
            if (useLocal)
            {
                LocalAccount selected = selectedAccount ?? throw new InvalidDataException("Select an account.");
                status.Text = experimentalHello || accounts.RequiresHello(selected.Id)
                    ? Localization.LoginText.Get("UnlockAccountWithHello")
                    : Localization.LoginText.Get("ReadingSavedAccount");
                byte[] credential = await WithHelloOwnerAsync(experimentalHello || accounts.RequiresHello(selected.Id),
                    () => accounts.UnlockAsync(selected.Id, handle, token, experimentalHello));
                SavedChannelCredential saved;
                try
                {
                    RequireCurrent(generation, token);
                    saved = StoredChannelIdentity.Decode(credential);
                    channel = saved.Identity;
                }
                finally
                {
                    CryptographicOperations.ZeroMemory(credential);
                }

                identity = await SavedChannelLogin.RestoreAsync(saved, api,
                    text =>
                    {
                        RequireCurrent(generation, token);
                        status.Text = text;
                    }, token);
            }
            else
            {
                status.Text = Localization.LoginText.Get("VerifyingSmsCode");
                channel = await api.LoginChannelAsync((phone.Text ?? "").Trim(), (captcha.Text ?? "").Trim(), token);
                RequireCurrent(generation, token);
                status.Text = Localization.LoginText.Get("ObtainingGameIdentity");
                identity = await api.ExchangeGameIdentityAsync(channel, token);
            }

            RequireCurrent(generation, token);
            phone.Clear();
            captcha.Clear();
            PendingAccountSave? pending = null;
            if (!useLocal || selectedAccount?.IdentityKey == null)
            {
                byte[] credential = StoredChannelIdentity.Encode(channel, api.DeviceId, api.Platform, api.ChannelId);
                try
                {
                    status.Text = experimentalHello
                        ? Localization.LoginText.Get("ProtectAccountWithHello")
                        : Localization.LoginText.Get("PreparingSavedAccount");
                    bool mayPrompt = experimentalHello || accounts.List().Any(a => accounts.RequiresHello(a.Id));
                    pending = await WithHelloOwnerAsync(mayPrompt,
                        () => accounts.TryPrepareChannelSaveAsync(label, credential, handle, token, experimentalHello,
                            save || useLocal));
                }
                finally
                {
                    CryptographicOperations.ZeroMemory(credential);
                }
            }

            if (pending != null)
                pending = await mainlandDisplays.AttachAsync(pending, new(0, MaskedPhone: useLocal ? null : label),
                    token);

            async Task Deliver()
            {
                RequireCurrent(generation, token);
                status.Text = Localization.LoginText.Get("CheckingAdmission");
                await deliver!(identity, channel, token);
            }

            LocalAccount? loggedIn = useLocal ? selectedAccount : null;
            if (pending != null)
            {
                loggedIn = await AccountSaveTransaction.SaveAndDeliverAsync(() => Task.FromResult(pending), Deliver);
                selectedAccount = loggedIn;
            }
            else await Deliver();

            RequireCurrent(generation, token);
            await remembered.SetAsync(loggedIn?.Id, token);
            autoResumeDisabled = false;
            requested = false;
            completion?.TrySetResult(true);
            HidePresented();
            ResetBackdrop();
            if (automaticAttempt && loggedIn != null) ShowWelcome(loggedIn.Label);
        }
        finally
        {
            identity?.Dispose();
            channel?.Dispose();
        }
    }

    internal void OnSdkLogout()
    {
        autoResumeDisabled = true;
        CancelFlow();
        rememberedChange = ClearRememberedAsync();
    }

    private async Task ClearRememberedAsync()
    {
        try
        {
            await remembered.SetAsync(null, CancellationToken.None);
        }
        catch (Exception)
        {
            status.Text = Localization.LoginText.Get("LogoutStateNotSaved");
        }
    }

    private static bool ReadProtectionPreference()
    {
        try
        {
            return NteLoginPreferences.ReadExperimentalHello(NteLoginPreferences.DefaultPath);
        }
        catch (Exception ex) when (ex is IOException or InvalidDataException or UnauthorizedAccessException
                                       or System.Text.Json.JsonException)
        {
            throw new ProbeAuthenticationException("login-preferences-unavailable", null);
        }
    }
}
