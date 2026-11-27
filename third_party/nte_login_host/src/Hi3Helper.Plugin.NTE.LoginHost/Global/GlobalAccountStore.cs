using System.Security.Cryptography;
using System.Text;
using Nte.Login;
using Nte.LoginHost.Accounts;

namespace Nte.LoginHost;

internal sealed class OverseasAccountStore
{
    internal static string DefaultDirectory => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "TouchUILaunch", "NTE", "Accounts", "Global");

    private Guid DeviceRecord => Region.HostRegion == NteHostRegion.Taiwan
        ? new("019bfe4d-f706-40b7-9114-3726a4c32013")
        : new("eddb1305-0b6e-4d17-bad2-9f03a57a201e");
    internal NteOverseasRegion Region { get; }
    internal static string DirectoryFor(NteOverseasRegion region) => Path.Combine(Path.GetDirectoryName(DefaultDirectory)!, region.Name);
    private readonly string directory;
    private readonly IAccountKeyProtector protector;
    private readonly LocalAccountStore vault;
    private readonly RememberedLogin remembered;
    private readonly AccountDisplayStore displays;

    internal OverseasAccountStore(string directory, IAccountKeyProtector protector,
        IAccountKeyProtector? helloProtector = null, NteOverseasRegion? region = null)
    {
        Region = region ?? NteOverseasRegion.Global;
        this.directory = Path.GetFullPath(directory);
        this.protector = protector;
        vault = new(this.directory, protector, helloProtector: helloProtector,
            subjectKey: (credential, key) => StoredGlobalIdentity.SubjectKey(credential, key, Region), credentialLimit: 12288);
        remembered = new(Path.Combine(this.directory, "last-login.bin"), protector);
        displays = new(this.directory, protector);
    }

    internal IReadOnlyList<LocalAccount> List() => vault.List();

    internal Task<AccountDisplayProfile?> DisplayAsync(LocalAccount account, CancellationToken token) =>
        displays.ReadAsync(account, token);

    internal bool RequiresHello(Guid id) => vault.RequiresHello(id);
    internal bool SaveMayPrompt(bool useHello) => useHello || List().Any(account => RequiresHello(account.Id));

    internal static bool ReadProtectionPreference(NteOverseasRegion? region = null)
    {
        try
        {
            return NteLoginPreferences.ReadExperimentalHello(NteLoginPreferences.GetPath((region ?? NteOverseasRegion.Global).HostRegion));
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException
                                          or System.Text.Json.JsonException)
        {
            throw new NteGlobalAuthenticationException("ProtectionSettingsUnavailable");
        }
    }

    internal Task<LocalAccount> RenameAsync(Guid id, string label, nint owner, CancellationToken token) =>
        vault.RenameAsync(id, label, owner, token);

    internal async Task RemoveAsync(Guid id, CancellationToken token)
    {
        if (await remembered.ReadAsync(token) == id) await remembered.SetAsync(null, token);
        displays.Remove(id);
        vault.Remove(id);
    }

    internal Task ClearRememberedAsync(CancellationToken token) => remembered.SetAsync(null, token);

    internal async Task<LocalAccount?> LastAsync(CancellationToken token) =>
        RememberedLogin.Resolve(await remembered.ReadAsync(token), remembered.HasState, List());

    internal async Task<string> DeviceIdAsync(CancellationToken token)
    {
        string path = Path.Combine(directory, "device.bin");
        if (File.Exists(path))
        {
            if (new FileInfo(path).Length is < 1 or > 4096)
                throw new InvalidDataException("Invalid Global device record.");
            byte[] plain = await protector.UnwrapAsync(DeviceRecord, File.ReadAllBytes(path), 0, token);
            try
            {
                string device = Encoding.ASCII.GetString(plain);
                if (device.Length != 32 || device.Any(c => !char.IsAsciiHexDigit(c)))
                    throw new InvalidDataException("Invalid Global device record.");
                return device;
            }
            finally
            {
                CryptographicOperations.ZeroMemory(plain);
            }
        }

        vault.EnsurePrivateDirectory();
        byte[] created = Encoding.ASCII.GetBytes(Guid.NewGuid().ToString("N"));
        try
        {
            byte[] wrapped = await protector.WrapAsync(DeviceRecord, created, 0, token);
            byte[] verified = await protector.UnwrapAsync(DeviceRecord, wrapped, 0, token);
            try
            {
                if (!CryptographicOperations.FixedTimeEquals(created, verified))
                    throw new CryptographicException("Global device verification failed.");
            }
            finally
            {
                CryptographicOperations.ZeroMemory(verified);
            }

            token.ThrowIfCancellationRequested();
            string temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                using (FileStream output = new(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None))
                {
                    await output.WriteAsync(wrapped, token);
                    output.Flush(true);
                }

                token.ThrowIfCancellationRequested();
                File.Move(temporary, path);
            }
            finally
            {
                if (File.Exists(temporary)) File.Delete(temporary);
            }

            return Encoding.ASCII.GetString(created);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(created);
        }
    }

    internal async Task<NteGlobalIdentity> RestoreAsync(Guid id, NteGlobalAuthentication api, nint owner,
        CancellationToken token,
        bool? useHello = null)
    {
        byte[] credential = await vault.UnlockAsync(id, owner, token, useHello);
        try
        {
            return await RefreshIdentityAsync(credential, api, token);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(credential);
        }
    }

    internal async Task RestoreAndDeliverAsync(Guid id, NteGlobalAuthentication api, bool remember, nint owner,
        Func<NteGlobalIdentity, CancellationToken, Task> deliver, CancellationToken token, bool useHello = false)
    {
        NteGlobalIdentity? identity = null;
        try
        {
            PendingAccountSave change = await vault.PrepareRefreshAsync(id, owner, async (credential, refreshToken) =>
            {
                identity = await RefreshIdentityAsync(credential, api, refreshToken);
                return StoredGlobalIdentity.Encode(identity);
            }, token, useHello);
            change = await displays.AttachAsync(change, Display(identity!), token);
            await AccountSaveTransaction.SaveAndDeliverAsync(() => Task.FromResult(change),
                () => deliver(identity!, token));
            token.ThrowIfCancellationRequested();
            await remembered.SetAsync(remember ? id : null, token);
        }
        finally
        {
            identity?.Dispose();
        }
    }

    private async Task<NteGlobalIdentity> RefreshIdentityAsync(byte[] credential, NteGlobalAuthentication api,
        CancellationToken token)
    {
        using NteGlobalIdentity saved = StoredGlobalIdentity.Decode(credential, Region);
        if (!api.Initialized) await api.InitializeAsync(token);
        NteGlobalIdentity restored = await api.RestoreSessionAsync(saved, token);
        if (NteGlobalIdentity.ReadUserId(restored.Result.GetProperty("uid")) !=
            NteGlobalIdentity.ReadUserId(saved.Result.GetProperty("uid")))
        {
            restored.Dispose();
            throw new InvalidDataException("Restored Global account does not match the selected record.");
        }

        return restored;
    }

    internal async Task<LocalAccount?> SaveAndDeliverAsync(NteGlobalIdentity identity, string label, bool remember,
        nint owner, Func<NteGlobalIdentity, CancellationToken, Task> deliver, CancellationToken token,
        bool useHello = false)
    {
        if (identity.Region != Region) throw new InvalidDataException("The account belongs to another publisher.");
        HashSet<Guid> existing = List().Select(account => account.Id).ToHashSet();
        byte[] credential = StoredGlobalIdentity.Encode(identity);
        PendingAccountSave? change;
        try
        {
            change = await vault.TryPrepareChannelSaveAsync(label, credential, owner, token, useHello, remember);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(credential);
        }

        if (change != null)
            change = await displays.AttachAsync(change,
                Display(identity) with
                {
                    InitialLabel = existing.Contains(change.Account.Id) ? null : change.Account.Label
                }, token);
        LocalAccount? account = null;
        if (change == null) await deliver(identity, token);
        else
            account = await AccountSaveTransaction.SaveAndDeliverAsync(() => Task.FromResult(change),
                () => deliver(identity, token));
        token.ThrowIfCancellationRequested();
        await remembered.SetAsync(remember ? account?.Id : null, token);
        return account;
    }

    private static AccountDisplayProfile Display(NteGlobalIdentity identity) =>
        new(identity.Provider, identity.Profile.Email, identity.Profile.Name, MaskedPhone: identity.Profile.MaskedPhone);
}
