using Nte.LoginHost.Accounts;

namespace Nte.LoginHost.Services;

internal sealed record GlobalAccountSnapshot(IReadOnlyList<LocalAccount> Accounts,
    IReadOnlyDictionary<Guid, AccountDisplayProfile> Profiles, Guid? LastAccount, bool DisplayUnavailable);

internal interface ITransientLoginSecret
{
    string Consume();
    void Clear();
}

internal interface IOverseasLoginService
{
    Nte.Login.NteOverseasRegion Region => Nte.Login.NteOverseasRegion.Global;
    bool SupportsEmail => Region.HostRegion == Nte.Login.NteHostRegion.Global;
    bool SupportsPhone => Region.HostRegion == Nte.Login.NteHostRegion.Taiwan;
    IReadOnlyList<Nte.Login.NtePhoneCountry> PhoneCountries => [];
    string DefaultCountryCode => "+886";
    bool Initialized { get; }
    TimeSpan CaptchaCooldown { get; }
    IReadOnlyList<LocalAccount> ListAccounts();
    Task InitializeAsync(CancellationToken token);
    Task<GlobalAccountSnapshot> LoadAccountsAsync(CancellationToken token);
    bool RequiresHello(Guid id);
    Task RestoreAsync(Guid id, CancellationToken token);
    Task RenameAsync(Guid id, string label, CancellationToken token);
    Task RemoveAsync(Guid id, CancellationToken token);
    Task SendCodeAsync(string email, CancellationToken token);
    Task SignInEmailAsync(string email, string secret, bool password, bool remember, CancellationToken token);
    Task SignInBrowserAsync(int provider, bool remember, CancellationToken token);
    Task SendPhoneCodeAsync(string countryCode, string phone, CancellationToken token) => Task.FromException(new InvalidOperationException("Phone authentication is unavailable."));
    Task SignInPhoneAsync(string countryCode, string phone, string secret, bool password, bool remember, CancellationToken token) => Task.FromException(new InvalidOperationException("Phone authentication is unavailable."));
}
