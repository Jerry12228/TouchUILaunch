using Nte.Login;
using Nte.LoginHost.Accounts;
using Nte.LoginHost.Localization;
using System.Text.Json;

namespace Nte.LoginHost.Services;

internal sealed class OverseasLoginService(NteGlobalAuthentication api, OverseasAccountStore? accounts, nint owner,
    Func<NteGlobalIdentity, CancellationToken, Task> deliver, Func<bool, Func<Task>, Task> protectOperation) : IOverseasLoginService
{
    public NteOverseasRegion Region => api.Region;
    public bool Initialized => api.Initialized && (!api.SupportsPhone || api.PhoneCountries.Count != 0);
    public IReadOnlyList<NtePhoneCountry> PhoneCountries => api.PhoneCountries;
    public string DefaultCountryCode => api.DefaultCountryCode;
    public TimeSpan CaptchaCooldown => api.CaptchaCooldown;
    public IReadOnlyList<LocalAccount> ListAccounts() => accounts?.List() ?? [];
    public async Task InitializeAsync(CancellationToken token)
    {
        if (!api.Initialized) await api.InitializeAsync(token);
        if (api.SupportsPhone && api.PhoneCountries.Count == 0) await api.LoadPhoneCountriesAsync(token);
    }
    public async Task<GlobalAccountSnapshot> LoadAccountsAsync(CancellationToken token)
    {
        if (accounts == null) return new([], new Dictionary<Guid, AccountDisplayProfile>(), null, false);
        IReadOnlyList<LocalAccount> entries = accounts.List();
        Dictionary<Guid, AccountDisplayProfile> profiles = [];
        bool unavailable = false;
        foreach (LocalAccount account in entries)
        {
            try { if (await accounts.DisplayAsync(account, token) is { } profile) profiles[account.Id] = profile; }
            catch (Exception error) when (AccountDisplayStore.IsDisplayError(error)) { unavailable = true; }
        }
        return new(entries, profiles, (await accounts.LastAsync(token))?.Id, unavailable);
    }
    public bool RequiresHello(Guid id) => accounts != null && (OverseasAccountStore.ReadProtectionPreference(api.Region) || accounts.RequiresHello(id));
    public Task RestoreAsync(Guid id, CancellationToken token)
    {
        if (accounts == null) throw new InvalidOperationException("No account store is available.");
        bool hello = OverseasAccountStore.ReadProtectionPreference(api.Region);
        return protectOperation(hello || accounts.RequiresHello(id), () => accounts.RestoreAndDeliverAsync(id, api, true, owner, deliver, token, hello));
    }
    public Task RenameAsync(Guid id, string label, CancellationToken token)
    {
        if (accounts == null) throw new InvalidOperationException("No account store is available.");
        return protectOperation(accounts.RequiresHello(id), async () => { await accounts.RenameAsync(id, label, owner, token); });
    }
    public Task RemoveAsync(Guid id, CancellationToken token) => accounts?.RemoveAsync(id, token) ?? Task.CompletedTask;
    public Task SendCodeAsync(string email, CancellationToken token) => api.SendEmailCaptchaAsync(email, token);
    public Task SendPhoneCodeAsync(string countryCode, string phone, CancellationToken token) => api.SendPhoneCaptchaAsync(countryCode, phone, token);
    public async Task SignInPhoneAsync(string countryCode, string phone, string secret, bool password, bool remember, CancellationToken token)
    {
        using NteGlobalIdentity identity = await api.LoginPhoneAsync(countryCode, phone, secret, password, token);
        await DeliverAsync(identity, remember, token);
    }
    public async Task SignInEmailAsync(string email, string secret, bool password, bool remember, CancellationToken token)
    {
        using NteGlobalIdentity identity = await api.LoginEmailAsync(email, secret, password, token);
        await DeliverAsync(identity, remember, token);
    }
    public async Task SignInBrowserAsync(int provider, bool remember, CancellationToken token)
    {
        using GlobalBrowserLogin browser = new(region: api.Region);
        using CancellationTokenSource timeout = CancellationTokenSource.CreateLinkedTokenSource(token);
        timeout.CancelAfter(TimeSpan.FromMinutes(10));
        using JsonDocument response = await browser.WaitAsync(api.CreateBrowserLoginUri(provider, browser.Target), timeout.Token);
        using NteGlobalIdentity identity = await api.CompleteBrowserLoginAsync(response.RootElement, provider, timeout.Token);
        await DeliverAsync(identity, remember, timeout.Token);
    }
    private async Task DeliverAsync(NteGlobalIdentity identity, bool remember, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        if (accounts == null) { await deliver(identity, token); return; }
        bool hello = OverseasAccountStore.ReadProtectionPreference(api.Region);
        string uid = NteGlobalIdentity.ReadUserId(identity.Result.GetProperty("uid"));
        string label = identity.Profile.Name ?? identity.Profile.Email ?? identity.Profile.MaskedPhone ?? LoginText.Get("SavedAccountNumber", uid[^Math.Min(6, uid.Length)..]);
        if (label.Length > 32) label = label[..(char.IsHighSurrogate(label[31]) ? 31 : 32)];
        await protectOperation(accounts.SaveMayPrompt(hello), () => accounts.SaveAndDeliverAsync(identity, label, remember, owner, deliver, token, hello));
    }
    internal static string PresentError(Exception error) => error switch
    {
        NteGlobalAuthenticationException authentication => LoginText.Get(authentication.Key, authentication.Arguments),
        HelloProtectionException hello => LoginText.Get("HelloProtectionFailed", $"0x{hello.NativeError:X8}"),
        _ => LoginText.Get("GlobalSignInFailed")
    };
}
