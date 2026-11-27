using Nte.LoginHost.Accounts;
using Nte.LoginHost.Services;
using System;
using System.Collections.Generic;
using System.Globalization;
using System.Linq;
using System.Net.Mail;
using System.Threading;
using System.Threading.Tasks;

namespace Nte.LoginHost.ViewModels;

internal enum GlobalLoginPage { Saved, Methods, Email, Phone, Help }

internal sealed class OverseasLoginViewModel : ObservableObject, IDisposable
{
    private readonly IOverseasLoginService service;
    private readonly ITransientLoginSecret secret;
    private readonly Func<string, string> text;
    private readonly Func<Exception, string> errorText;
    private readonly string? initialNotice;
    private readonly SynchronizationContext? presentationContext = SynchronizationContext.Current;
    private readonly TaskCompletionSource<bool> completion = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private CancellationTokenSource? lifetime, operation;
    private Task work = Task.CompletedTask;
    private GlobalLoginPage page, previousPage;
    private string email = "", emailCode = "", status = "", remark = "";
    private string phone = "", countryCode = "+886";
    private bool busy, password = true, remember = true, agreement;
    private Guid? selectedAccount;
    private DateTimeOffset previewNextCode;
    public Nte.Login.NteOverseasRegion Region => service.Region;
    public string Title => text(Region.HostRegion == Nte.Login.NteHostRegion.Taiwan ? "TaiwanTitle" : "GlobalTitle");
    public bool SupportsEmail => service.SupportsEmail;
    public bool SupportsPhone => service.SupportsPhone;
    public IReadOnlyList<Nte.Login.NtePhoneCountry> PhoneCountries => IsPreview ? Nte.Login.NtePhoneCountry.Preview : service.PhoneCountries;
    public string CountryCode { get => countryCode; set { if (Set(ref countryCode, value ?? service.DefaultCountryCode)) ClearCredentials(); } }
    public string Phone { get => phone; set { if (Set(ref phone, value)) ClearCredentials(); } }
    public string Identifier { get => Page == GlobalLoginPage.Phone ? Phone : Email; set { if (Page == GlobalLoginPage.Phone) Phone = value; else Email = value; } }
    public bool IsPreview { get; }
    public bool IsBusy { get => busy; private set { Set(ref busy, value); Refresh(); } }
    public bool Ready => IsPreview || service.Initialized;
    public bool CanEdit => !IsBusy;
    public string Email { get => email; set { if (Set(ref email, value)) { EmailCode = ""; secret.Clear(); Refresh(); } } }
    public string EmailCode { get => emailCode; set => Set(ref emailCode, value); }
    public string Remark { get => remark; set => Set(ref remark, value); }
    public bool UsePassword { get => password; set { if (Set(ref password, value)) { ClearCredentials(); Refresh(); } } }
    public bool Remember { get => remember; set => Set(ref remember, value); }
    public bool Agreement { get => agreement; set => Set(ref agreement, value); }
    public string Status { get => status; private set => Set(ref status, value); }
    public Guid? SelectedAccountId { get => selectedAccount; set { Set(ref selectedAccount, value); Refresh(); } }
    public GlobalLoginPage Page { get => page; private set { Set(ref page, value); Notify(nameof(Identifier)); Refresh(); } }
    public IReadOnlyList<LocalAccount> Accounts { get; private set; }
    public IReadOnlyDictionary<Guid, AccountDisplayProfile> Profiles { get; private set; } = new Dictionary<Guid, AccountDisplayProfile>();
    public TimeSpan CodeCooldown => IsPreview ? (previewNextCode > DateTimeOffset.UtcNow ? previewNextCode - DateTimeOffset.UtcNow : TimeSpan.Zero) : service.CaptchaCooldown;
    public string CaptchaLabel => CodeCooldown > TimeSpan.Zero
        ? string.Format(CultureInfo.CurrentUICulture, text("SmsResendCountdown"), (int)Math.Ceiling(CodeCooldown.TotalSeconds)) : text(Page == GlobalLoginPage.Phone ? "SendSmsCode" : "SendEmailCode");
    public string SubmitLabel => text(!Ready ? "RetryConnection" : Page == GlobalLoginPage.Saved ? "ContinueSignIn" : Page == GlobalLoginPage.Phone ? "PhoneSignIn" : "EmailSignIn");
    public AsyncRelayCommand RetryCommand { get; }
    public AsyncRelayCommand SubmitCommand { get; }
    public AsyncRelayCommand SendCodeCommand { get; }
    public AsyncRelayCommand GoogleCommand { get; }
    public AsyncRelayCommand FacebookCommand { get; }
    public AsyncRelayCommand RenameCommand { get; }
    public AsyncRelayCommand RemoveCommand { get; }
    public RelayCommand MethodsCommand { get; }
    public RelayCommand EmailCommand { get; }
    public RelayCommand PhoneCommand { get; }
    public RelayCommand BackCommand { get; }
    public RelayCommand HelpCommand { get; }
    public RelayCommand CancelOperationCommand { get; }
    internal event Action? CredentialsCleared;

    internal OverseasLoginViewModel(IOverseasLoginService service, ITransientLoginSecret secret,
        Func<string, string> text, Func<Exception, string> errorText, bool preview = false, string? notice = null)
    {
        this.service = service; this.secret = secret; this.text = text; this.errorText = errorText;
        initialNotice = notice; IsPreview = preview;
        Accounts = preview ? [] : service.ListAccounts();
        selectedAccount = Accounts.FirstOrDefault()?.Id;
        page = Accounts.Count > 0 ? GlobalLoginPage.Saved : GlobalLoginPage.Methods;
        RetryCommand = new(token => ExecuteAsync(InitializeAsync, token), () => !IsBusy);
        SubmitCommand = new(SubmitAsync, () => !IsBusy && (Page is GlobalLoginPage.Email or GlobalLoginPage.Phone || Page == GlobalLoginPage.Saved && (SelectedAccountId != null || !Ready)));
        SendCodeCommand = new(SendCodeAsync, () => !IsBusy && Ready && (Page is GlobalLoginPage.Email or GlobalLoginPage.Phone) && CodeCooldown == TimeSpan.Zero);
        GoogleCommand = new(token => BrowserAsync(2, token), () => !IsBusy && Ready);
        FacebookCommand = new(token => BrowserAsync(1, token), () => !IsBusy && Ready);
        RenameCommand = new(RenameAsync, () => !IsBusy && SelectedAccountId != null);
        RemoveCommand = new(RemoveAsync, () => !IsBusy && SelectedAccountId != null);
        MethodsCommand = new(() => Navigate(GlobalLoginPage.Methods), () => !IsBusy);
        EmailCommand = new(() => Navigate(GlobalLoginPage.Email), () => !IsBusy && Ready && SupportsEmail);
        PhoneCommand = new(() => { UsePassword = false; Navigate(GlobalLoginPage.Phone); }, () => !IsBusy && Ready && SupportsPhone);
        BackCommand = new(() => Navigate(Page is GlobalLoginPage.Email or GlobalLoginPage.Phone ? GlobalLoginPage.Methods : Page == GlobalLoginPage.Help ? previousPage : GlobalLoginPage.Saved), () => !IsBusy);
        HelpCommand = new(ShowHelp, () => !IsBusy);
        CancelOperationCommand = new(() => operation?.Cancel(), () => IsBusy);
    }

    internal async Task<bool> RunAsync(CancellationToken token)
    {
        lifetime = CancellationTokenSource.CreateLinkedTokenSource(token);
        using var registration = token.Register(() =>
        {
            if (presentationContext != null) presentationContext.Post(_ => Cancel(), null); else Cancel();
        });
        try
        {
            if (IsPreview) { Status = initialNotice ?? ""; Refresh(); }
            else await ExecuteAsync(InitializeAsync, token);
            return await completion.Task;
        }
        finally { lifetime.Cancel(); operation?.Cancel(); await work; lifetime.Dispose(); lifetime = null; }
    }

    private async Task InitializeAsync(CancellationToken token)
    {
        Status = text("Connecting");
        if (!service.Initialized) await service.InitializeAsync(token);
        if (SupportsPhone) { CountryCode = service.DefaultCountryCode; Notify(nameof(PhoneCountries)); }
        Status = initialNotice ?? "";
        await ReloadAccountsAsync(token);
    }

    private async Task ReloadAccountsAsync(CancellationToken token, Guid? wanted = null)
    {
        GlobalAccountSnapshot snapshot = await service.LoadAccountsAsync(token);
        Accounts = snapshot.Accounts; Profiles = snapshot.Profiles;
        SelectedAccountId = wanted ?? SelectedAccountId ?? snapshot.LastAccount ?? Accounts.FirstOrDefault()?.Id;
        if (!Accounts.Any(account => account.Id == SelectedAccountId)) SelectedAccountId = Accounts.FirstOrDefault()?.Id;
        Notify(nameof(Profiles)); Notify(nameof(Accounts));
        if (snapshot.DisplayUnavailable) Status = text("DisplayInfoUnavailable");
    }

    private Task SubmitAsync(CancellationToken token)
    {
        if (!Ready) return ExecuteAsync(InitializeAsync, token);
        if (Page == GlobalLoginPage.Saved && SelectedAccountId is { } id)
            return ExecuteAsync(async ct =>
            {
                Status = text(service.RequiresHello(id) ? "UnlockAccountWithHello" : "ReadingSavedAccount");
                await service.RestoreAsync(id, ct);
                ct.ThrowIfCancellationRequested(); completion.TrySetResult(true);
            }, token);
        if (Page == GlobalLoginPage.Phone) return SubmitPhoneAsync(token);
        if (Page != GlobalLoginPage.Email || !SupportsEmail || !ValidateAgreementAndEmail()) return Task.CompletedTask;
        if (IsPreview) { Status = text("PreviewNoAuthentication"); return Task.CompletedTask; }
        string value = UsePassword ? secret.Consume() : EmailCode.Trim();
        if (value.Length == 0) { Status = text(UsePassword ? "EnterPassword" : "EnterEmailCode"); return Task.CompletedTask; }
        string address = Email.Trim(); bool usePassword = UsePassword, save = Remember;
        return ExecuteAsync(async ct =>
        {
            await service.SignInEmailAsync(address, value, usePassword, save, ct);
            ClearCredentials(); ct.ThrowIfCancellationRequested(); completion.TrySetResult(true);
        }, token);
    }

    private Task SendCodeAsync(CancellationToken token)
    {
        if (Page == GlobalLoginPage.Phone)
        {
            if (!ValidatePhone()) return Task.CompletedTask;
            if (IsPreview) { previewNextCode = DateTimeOffset.UtcNow.AddSeconds(60); Status = text("PreviewNoAuthentication"); Refresh(); return Task.CompletedTask; }
            string code = CountryCode, number = Phone.Trim();
            return ExecuteAsync(async ct => { await service.SendPhoneCodeAsync(code, number, ct); Status = text("SmsCodeSent"); Refresh(); }, token);
        }
        if (Page != GlobalLoginPage.Email || !SupportsEmail) return Task.CompletedTask;
        if (!ValidateAgreementAndEmail()) return Task.CompletedTask;
        if (IsPreview) { previewNextCode = DateTimeOffset.UtcNow.AddSeconds(60); Status = text("PreviewNoAuthentication"); Refresh(); return Task.CompletedTask; }
        string address = Email.Trim();
        return ExecuteAsync(async ct => { await service.SendCodeAsync(address, ct); Status = text("EmailCodeSent"); Refresh(); }, token);
    }

    private Task SubmitPhoneAsync(CancellationToken token)
    {
        if (!ValidatePhone()) return Task.CompletedTask;
        if (IsPreview) { Status = text("PreviewNoAuthentication"); return Task.CompletedTask; }
        if (!UsePassword && !System.Text.RegularExpressions.Regex.IsMatch(EmailCode.Trim(), @"\A[0-9]{4,8}\z"))
        { Status = text("InvalidSmsCode"); return Task.CompletedTask; }
        string value = UsePassword ? secret.Consume() : EmailCode.Trim();
        if (value.Length == 0) { Status = text("EnterPassword"); return Task.CompletedTask; }
        string code = CountryCode, number = Phone.Trim(); bool passwordMode = UsePassword, save = Remember;
        return ExecuteAsync(async ct =>
        {
            await service.SignInPhoneAsync(code, number, value, passwordMode, save, ct);
            ClearCredentials(); ct.ThrowIfCancellationRequested(); completion.TrySetResult(true);
        }, token);
    }

    private bool ValidatePhone()
    {
        if (!SupportsPhone) { Status = text("LoginMethodUnavailable"); return false; }
        if (!Agreement) { Status = text("AgreementRequired"); return false; }
        if (!PhoneCountries.Any(item => item.CountryCode == CountryCode) || !System.Text.RegularExpressions.Regex.IsMatch(Phone.Trim(), @"\A[0-9]{6,15}\z"))
        { Status = text("InvalidOverseasPhone"); return false; }
        return true;
    }

    private Task BrowserAsync(int provider, CancellationToken token)
    {
        if (!Agreement) { Status = text("AgreementRequired"); return Task.CompletedTask; }
        bool save = Remember;
        return ExecuteAsync(async ct =>
        {
            Status = text("BrowserPending"); await service.SignInBrowserAsync(provider, save, ct);
            ct.ThrowIfCancellationRequested(); completion.TrySetResult(true);
        }, token);
    }

    private Task RenameAsync(CancellationToken token)
    {
        if (SelectedAccountId is not { } id) return Task.CompletedTask;
        string label = Remark.Trim();
        if (label.Length is < 1 or > 32 || label.Any(char.IsControl)) { Status = text("InvalidLabel"); return Task.CompletedTask; }
        if (IsPreview)
        {
            Accounts = Accounts.Select(account => account.Id == id ? account with { Label = label } : account).ToArray();
            Notify(nameof(Accounts)); Status = text("LabelUpdated"); return Task.CompletedTask;
        }
        return ExecuteAsync(async ct => { await service.RenameAsync(id, label, ct); await ReloadAccountsAsync(ct, id); Status = text("LabelUpdated"); }, token);
    }

    private Task RemoveAsync(CancellationToken token)
    {
        if (SelectedAccountId is not { } id) return Task.CompletedTask;
        if (IsPreview)
        {
            Accounts = Accounts.Where(account => account.Id != id).ToArray(); SelectedAccountId = Accounts.FirstOrDefault()?.Id;
            Notify(nameof(Accounts)); if (Accounts.Count == 0) Navigate(GlobalLoginPage.Methods); return Task.CompletedTask;
        }
        return ExecuteAsync(async ct =>
        {
            await service.RemoveAsync(id, ct); await ReloadAccountsAsync(ct);
            if (Accounts.Count == 0) Page = GlobalLoginPage.Methods;
            Status = text("AccountRemoved");
        }, token);
    }

    private bool ValidateAgreementAndEmail()
    {
        if (!Agreement) { Status = text("AgreementRequired"); return false; }
        string value = Email.Trim();
        if (value.Length > 128 || !MailAddress.TryCreate(value, out MailAddress? address) || address.Address != value)
        { Status = text("InvalidEmail"); return false; }
        return true;
    }

    private Task ExecuteAsync(Func<CancellationToken, Task> action, CancellationToken token)
    {
        if (IsBusy || lifetime == null || completion.Task.IsCompleted) return Task.CompletedTask;
        if (IsPreview) { Status = text("PreviewNoAuthentication"); return Task.CompletedTask; }
        IsBusy = true;
        operation = CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token, token);
        return work = RunActionAsync();
        async Task RunActionAsync()
        {
            try { await action(operation.Token); }
            catch (OperationCanceledException) { if (!completion.Task.IsCompleted) Status = text("AuthorizationCancelled"); }
            catch (Exception error) { Status = errorText(error); }
            finally { operation.Dispose(); operation = null; IsBusy = false; }
        }
    }

    internal void SetPreviewBusy(bool value) { if (!IsPreview) throw new InvalidOperationException("Synthetic login state only."); IsBusy = value; }
    internal void Tick() { Notify(nameof(CaptchaLabel)); SendCodeCommand.Refresh(); }
    internal void ShowNotice(string notice) => Status = notice;
    internal void ShowHelp()
    {
        if (IsBusy) return;
        if (Page == GlobalLoginPage.Help) { Navigate(previousPage); return; }
        previousPage = Page; Navigate(GlobalLoginPage.Help);
    }
    internal void Navigate(GlobalLoginPage next)
    {
        if (IsBusy) return;
        if (next == GlobalLoginPage.Email && !SupportsEmail || next == GlobalLoginPage.Phone && !SupportsPhone) return;
        if (next == GlobalLoginPage.Saved && Accounts.Count == 0) next = GlobalLoginPage.Methods;
        if (Page != next) ClearCredentials();
        Page = next; Status = "";
    }
    private void ClearCredentials() { secret.Clear(); EmailCode = ""; CredentialsCleared?.Invoke(); }
    internal void Cancel() { lifetime?.Cancel(); operation?.Cancel(); ClearCredentials(); completion.TrySetResult(false); }
    private void Refresh()
    {
        Notify(nameof(CanEdit)); Notify(nameof(Ready)); Notify(nameof(CaptchaLabel)); Notify(nameof(SubmitLabel));
        RetryCommand?.Refresh(); SubmitCommand?.Refresh(); SendCodeCommand?.Refresh(); GoogleCommand?.Refresh(); FacebookCommand?.Refresh();
        RenameCommand?.Refresh(); RemoveCommand?.Refresh(); MethodsCommand?.Refresh(); EmailCommand?.Refresh(); PhoneCommand?.Refresh(); BackCommand?.Refresh(); HelpCommand?.Refresh(); CancelOperationCommand?.Refresh();
    }

    internal void SetPreviewAccounts(int count)
    {
        if (!IsPreview || count is < 0 or > 16) throw new InvalidOperationException("Synthetic accounts only.");
        Dictionary<Guid, AccountDisplayProfile> display = []; List<LocalAccount> entries = [];
        for (int i = 0; i < count; i++)
        {
            Guid id = new(i + 1, 0, 0, new byte[8]);
            string address = i == 2 ? "a.long.account.name.for.layout.testing@example.com" : $"user{i + 1}@example.com";
            entries.Add(new(id, "Helix", DateTimeOffset.UtcNow, null));
            display[id] = new(Region.HostRegion == Nte.Login.NteHostRegion.Taiwan ? (i % 2 == 0 ? 2 : 1) : i < 2 ? 2 : i % 2 == 0 ? 8 : 1, address, "Helix", InitialLabel: "Helix");
        }
        Accounts = entries; Profiles = display; SelectedAccountId = entries.FirstOrDefault()?.Id;
        Page = count == 0 ? GlobalLoginPage.Methods : GlobalLoginPage.Saved;
        Notify(nameof(Profiles)); Notify(nameof(Accounts));
    }
    internal void SetPreviewProfileScenario(string variant)
    {
        if (!IsPreview) throw new InvalidOperationException("Synthetic accounts only.");
        Dictionary<Guid, AccountDisplayProfile> profiles = new(Profiles);
        if (variant == "legacy-profile")
        {
            Accounts = Accounts.Select((account, index) => account with { Label = "Legacy label " + (index + 1) }).ToArray();
            foreach (LocalAccount account in Accounts) profiles[account.Id] = new(2, Email: "user" + account.Id.ToString("N")[..8] + "@example.com", Name: "Helix");
        }
        else if (variant == "legacy") { profiles.Clear(); Accounts = Accounts.Select(account => account with { Label = "Google" }).ToArray(); }
        else if (variant == "missing") foreach (LocalAccount account in Accounts) profiles[account.Id] = new(2, Name: "Helix", InitialLabel: account.Label);
        else throw new InvalidOperationException("Unknown preview profile scenario.");
        Profiles = profiles; Notify(nameof(Profiles)); Notify(nameof(Accounts));
    }
    public void Dispose()
    {
        Cancel(); RetryCommand.Dispose(); SubmitCommand.Dispose(); SendCodeCommand.Dispose(); GoogleCommand.Dispose(); FacebookCommand.Dispose(); RenameCommand.Dispose(); RemoveCommand.Dispose();
    }
}
