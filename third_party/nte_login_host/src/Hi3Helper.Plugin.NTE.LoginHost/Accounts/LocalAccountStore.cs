using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Nte.LoginHost.Accounts;

internal static class AccountProtection
{
    internal const string Hello = "Passport-RSA2048-AES256GCM-v1";
    internal const string CurrentUser = "DPAPI-CurrentUser-AES256GCM-v1";
}

internal interface IAccountKeyProtector
{
    string Protection { get; }
    Task<byte[]> WrapAsync(Guid account, byte[] key, nint owner, CancellationToken token);
    Task<byte[]> UnwrapAsync(Guid account, byte[] wrappedKey, nint owner, CancellationToken token);
    void Delete(Guid account);
}

internal sealed record LocalAccount(
    Guid Id,
    string Label,
    DateTimeOffset SavedUtc,
    DateTimeOffset? ExpiresUtc,
    string? IdentityKey = null)
{
    public override string ToString() => Label;
}

internal sealed class PendingAccountSave(LocalAccount account, Action rollback, Action commit) : IDisposable
{
    internal LocalAccount Account { get; } = account;
    private bool complete;

    internal void Commit()
    {
        complete = true;
        commit();
    }

    public void Dispose()
    {
        if (!complete)
        {
            rollback();
            complete = true;
        }
    }
}

// Every account has an independent AES key. Its explicit protection kind is
// authenticated with the metadata; a failed Hello operation never falls back.
internal sealed partial class LocalAccountStore(
    string directory,
    IAccountKeyProtector protector,
    TimeProvider? clock = null,
    IAccountKeyProtector? helloProtector = null,
    Func<byte[], byte[], string>? subjectKey = null,
    int credentialLimit = 1024)
{
    private readonly string root = Path.GetFullPath(directory);
    private readonly Func<byte[], byte[], string> identityKey = subjectKey ?? StoredChannelIdentity.SubjectKey;

    private readonly int maximumCredentialBytes = credentialLimit is > 0 and <= 12288
        ? credentialLimit
        : throw new ArgumentOutOfRangeException(nameof(credentialLimit));

    private readonly TimeProvider time = clock ?? TimeProvider.System;

    private sealed record Record(
        int Format,
        string Protection,
        LocalAccount Account,
        byte[] WrappedKey,
        byte[] Nonce,
        byte[] Tag,
        byte[] Ciphertext);

    internal IReadOnlyList<LocalAccount> List()
    {
        if (!Directory.Exists(root)) return [];
        List<LocalAccount> accounts = [];
        foreach (string path in Directory.EnumerateFiles(root, "*.json").Take(64))
        {
            if (!Guid.TryParseExact(Path.GetFileNameWithoutExtension(path), "N", out Guid id)) continue;
            try
            {
                accounts.Add(Read(id).Account);
            }
            catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or JsonException
                                           or InvalidDataException)
            {
            }
        }

        return accounts.OrderByDescending(a => a.SavedUtc).ToArray();
    }

    internal bool RequiresHello(Guid id) => Read(id).Protection == AccountProtection.Hello;

    internal async Task<PendingAccountSave> PrepareChannelSaveAsync(string label, byte[] credential, nint owner,
        CancellationToken token, bool useHello = false) =>
        await TryPrepareChannelSaveAsync(label, credential, owner, token, useHello, true).ConfigureAwait(false) ??
        throw new InvalidOperationException("Account preparation failed.");

    internal async Task<PendingAccountSave?> TryPrepareChannelSaveAsync(string label, byte[] credential, nint owner,
        CancellationToken token, bool useHello, bool createIfMissing)
    {
        label = ValidateLabel(label);
        if (credential.Length < 1 || credential.Length > maximumCredentialBytes)
            throw new InvalidDataException("Invalid credential size.");
        byte[] subjectSecret = await SubjectSecretAsync(token).ConfigureAwait(false);
        List<Record> matches = [];
        string subject;
        try
        {
            subject = identityKey(credential, subjectSecret);
            foreach (LocalAccount account in List())
            {
                Record record = Read(account.Id);
                if (record.Format == 1) continue; // Do not silently extend a legacy local lease.
                if (account.IdentityKey != null)
                {
                    if (account.IdentityKey == subject) matches.Add(record);
                    continue;
                }

                byte[] prior = await UnlockAsync(account.Id, owner, token).ConfigureAwait(false);
                try
                {
                    if (identityKey(prior, subjectSecret) == subject) matches.Add(record);
                }
                catch (InvalidDataException)
                {
                    /* A legacy payload may require fresh authentication. */
                }
                finally
                {
                    CryptographicOperations.ZeroMemory(prior);
                }
            }
        }
        finally
        {
            CryptographicOperations.ZeroMemory(subjectSecret);
        }

        Record? original = matches.OrderBy(r =>
                System.Text.RegularExpressions.Regex.IsMatch(r.Account.Label, @"\A1[0-9]{2}\*{4}[0-9]{4}\z"))
            .ThenBy(r => r.Account.SavedUtc).FirstOrDefault();
        if (original == null && !createIfMissing) return null;
        if (original == null && List().Count >= 16) throw new InvalidDataException("Local account capacity reached.");
        LocalAccount saved = original == null
            ? new(Guid.NewGuid(), label, time.GetUtcNow(), null, subject)
            : original.Account with { IdentityKey = subject };
        IAccountKeyProtector desired = SelectDesired(useHello);
        IAccountKeyProtector? previous = original == null ? null : Select(original.Protection);
        byte[] key = [], wrapped = [], priorPlain = [];
        bool written = false, newProtection = original == null || original.Protection != desired.Protection;
        try
        {
            if (original == null) key = RandomNumberGenerator.GetBytes(32);
            else
            {
                key = await previous!.UnwrapAsync(saved.Id, original.WrappedKey, owner, token).ConfigureAwait(false);
                priorPlain = new byte[original.Ciphertext.Length];
                Decrypt(original, key, priorPlain);
            }

            wrapped = newProtection
                ? await WrapAndVerifyAsync(desired, saved.Id, key, owner, token).ConfigureAwait(false)
                : original!.WrappedKey;
            Write(Encrypt(3, desired.Protection, saved, wrapped, credential, key), token, overwrite: original != null);
            written = true;
            return new(saved, () =>
            {
                if (original == null) Remove(saved.Id);
                else
                {
                    Write(original, CancellationToken.None, overwrite: true);
                    if (newProtection) desired.Delete(saved.Id);
                }
            }, () =>
            {
                if (previous != null && newProtection) previous.Delete(saved.Id);
                foreach (Record duplicate in matches.Where(r => r.Account.Id != saved.Id)) Remove(duplicate.Account.Id);
            });
        }
        finally
        {
            CryptographicOperations.ZeroMemory(key);
            CryptographicOperations.ZeroMemory(priorPlain);
            if (!written && newProtection) desired.Delete(saved.Id);
        }
    }

    private static readonly Guid SubjectSecretId = new("95612a47-bbc5-46dd-af49-419d37e2fbd8");

    private async Task<byte[]> SubjectSecretAsync(CancellationToken token)
    {
        string path = Path.Combine(root, "subject-key.bin");
        IAccountKeyProtector currentUser = Select(AccountProtection.CurrentUser);
        if (File.Exists(path))
        {
            if (new FileInfo(path).Length is < 1 or > 4096)
                throw new InvalidDataException("Invalid account index protection.");
            byte[] key = await currentUser.UnwrapAsync(SubjectSecretId, File.ReadAllBytes(path), 0, token)
                .ConfigureAwait(false);
            if (key.Length == 32) return key;
            CryptographicOperations.ZeroMemory(key);
            throw new CryptographicException("Invalid account index key.");
        }

        EnsurePrivateDirectory();
        byte[] created = RandomNumberGenerator.GetBytes(32);
        try
        {
            byte[] wrapped = await WrapAndVerifyAsync(currentUser, SubjectSecretId, created, 0, token)
                .ConfigureAwait(false);
            using FileStream output = new(path, FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096,
                FileOptions.WriteThrough);
            token.ThrowIfCancellationRequested();
            output.Write(wrapped);
            output.Flush(true);
            return created.ToArray();
        }
        finally
        {
            CryptographicOperations.ZeroMemory(created);
        }
    }

    internal async Task<LocalAccount> SaveAsync(string label, byte[] credential, nint owner, CancellationToken token,
        bool useHello = false)
    {
        label = ValidateLabel(label);
        if (credential.Length < 1 || credential.Length > maximumCredentialBytes)
            throw new InvalidDataException("Invalid credential size.");
        if (List().Count >= 16) throw new InvalidDataException("Local account capacity reached.");
        // User-selected retention applies to new records; old leases stay intact.
        LocalAccount account = new(Guid.NewGuid(), label, time.GetUtcNow(), null);
        IAccountKeyProtector selected = SelectDesired(useHello);
        byte[] key = RandomNumberGenerator.GetBytes(32), wrapped = [];
        bool committed = false;
        try
        {
            wrapped = await WrapAndVerifyAsync(selected, account.Id, key, owner, token).ConfigureAwait(false);
            Record record = Encrypt(2, selected.Protection, account, wrapped, credential, key);
            Write(record, token, overwrite: false);
            committed = true;
            return account;
        }
        finally
        {
            CryptographicOperations.ZeroMemory(key);
            if (!committed) selected.Delete(account.Id);
        }
    }

    // Refresh a selected record while holding the key obtained by one unlock.
    // The refreshed credential and protection conversion are one rollbackable
    // change; re-opening the same Hello key for save would prompt a second time.
    internal async Task<PendingAccountSave> PrepareRefreshAsync(Guid id, nint owner,
        Func<byte[], CancellationToken, Task<byte[]>> refresh, CancellationToken token, bool? useHello = null)
    {
        Record original = Read(id);
        if (original.Account.ExpiresUtc is DateTimeOffset expires && expires <= time.GetUtcNow())
            throw new InvalidDataException("Local retention expired.");
        IAccountKeyProtector previous = Select(original.Protection);
        IAccountKeyProtector desired =
            useHello is bool enabled && original.Format >= 2 ? SelectDesired(enabled) : previous;
        byte[] key = [], plain = [], replacement = [];
        bool written = false, newProtection = previous.Protection != desired.Protection;
        try
        {
            key = await previous.UnwrapAsync(id, original.WrappedKey, owner, token).ConfigureAwait(false);
            token.ThrowIfCancellationRequested();
            plain = new byte[original.Ciphertext.Length];
            Decrypt(original, key, plain);
            replacement = await refresh(plain, token).ConfigureAwait(false);
            if (replacement.Length is < 1 || replacement.Length > maximumCredentialBytes)
                throw new InvalidDataException("Refreshed credential size bound.");
            // A metadata label cannot authorize replacing one account with another.
            if (original.Account.IdentityKey != null)
            {
                byte[] subjectSecret = await SubjectSecretAsync(token).ConfigureAwait(false);
                try
                {
                    if (identityKey(replacement, subjectSecret) != original.Account.IdentityKey)
                        throw new InvalidDataException("Refreshed account identity changed.");
                }
                finally
                {
                    CryptographicOperations.ZeroMemory(subjectSecret);
                }
            }

            byte[] wrapped = newProtection
                ? await WrapAndVerifyAsync(desired, id, key, owner, token).ConfigureAwait(false)
                : original.WrappedKey;
            Write(Encrypt(original.Format, desired.Protection, original.Account, wrapped, replacement, key), token,
                overwrite: true);
            written = true;
            return new(original.Account, () =>
            {
                Write(original, CancellationToken.None, overwrite: true);
                if (newProtection) desired.Delete(id);
            }, () =>
            {
                if (newProtection) previous.Delete(id);
            });
        }
        finally
        {
            CryptographicOperations.ZeroMemory(key);
            CryptographicOperations.ZeroMemory(plain);
            CryptographicOperations.ZeroMemory(replacement);
            if (!written && newProtection) desired.Delete(id);
        }
    }

    internal async Task<byte[]> UnlockAsync(Guid id, nint owner, CancellationToken token, bool? useHello = null)
    {
        Record record = Read(id);
        if (record.Account.ExpiresUtc is DateTimeOffset expires && expires <= time.GetUtcNow())
            throw new InvalidDataException("Local retention expired.");
        IAccountKeyProtector current = Select(record.Protection);
        byte[] key = [], plain = new byte[record.Ciphertext.Length];
        bool released = false;
        try
        {
            key = await current.UnwrapAsync(id, record.WrappedKey, owner, token).ConfigureAwait(false);
            token.ThrowIfCancellationRequested();
            Decrypt(record, key, plain);
            if (useHello is bool enabled && record.Format >= 2)
            {
                IAccountKeyProtector desired = SelectDesired(enabled);
                if (desired.Protection != current.Protection)
                {
                    bool committed = false;
                    try
                    {
                        byte[] wrapped = await WrapAndVerifyAsync(desired, id, key, owner, token).ConfigureAwait(false);
                        Write(Encrypt(record.Format, desired.Protection, record.Account, wrapped, plain, key), token,
                            overwrite: true);
                        committed = true;
                    }
                    finally
                    {
                        if (!committed) desired.Delete(id);
                    }

                    // The old protector is removed only after the new protected
                    // record is committed. Failure cannot silently bypass Hello.
                    current.Delete(id);
                }
            }

            token.ThrowIfCancellationRequested();
            released = true;
            return plain;
        }
        finally
        {
            CryptographicOperations.ZeroMemory(key);
            if (!released) CryptographicOperations.ZeroMemory(plain);
        }
    }

    internal async Task<LocalAccount> RenameAsync(Guid id, string label, nint owner, CancellationToken token)
    {
        label = ValidateLabel(label);
        Record record = Read(id);
        byte[] key = [], plain = new byte[record.Ciphertext.Length];
        try
        {
            key = await Select(record.Protection).UnwrapAsync(id, record.WrappedKey, owner, token)
                .ConfigureAwait(false);
            token.ThrowIfCancellationRequested();
            Decrypt(record, key, plain);
            LocalAccount renamed = record.Account with { Label = label };
            Write(Encrypt(record.Format, record.Protection, renamed, record.WrappedKey, plain, key), token,
                overwrite: true);
            return renamed;
        }
        finally
        {
            CryptographicOperations.ZeroMemory(key);
            CryptographicOperations.ZeroMemory(plain);
        }
    }

    internal void Remove(Guid id)
    {
        Record record = Read(id);
        Select(record.Protection).Delete(id);
        File.Delete(AccountPath(id));
    }

    private static async Task<byte[]> WrapAndVerifyAsync(IAccountKeyProtector selected, Guid id, byte[] key, nint owner,
        CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        byte[] wrapped = await selected.WrapAsync(id, key, owner, token).ConfigureAwait(false), verified = [];
        try
        {
            verified = await selected.UnwrapAsync(id, wrapped, owner, token).ConfigureAwait(false);
            token.ThrowIfCancellationRequested();
            if (verified.Length != 32 || !CryptographicOperations.FixedTimeEquals(key, verified))
                throw new CryptographicException("Protected key verification failed.");
            return wrapped;
        }
        finally
        {
            CryptographicOperations.ZeroMemory(verified);
        }
    }

    private static void Decrypt(Record record, byte[] key, byte[] plain)
    {
        if (key.Length != 32) throw new CryptographicException("Invalid protected key.");
        using AesGcm aes = new(key, 16);
        aes.Decrypt(record.Nonce, record.Ciphertext, record.Tag, plain,
            AssociatedData(record.Format, record.Protection, record.Account));
    }

    private static Record Encrypt(int format, string protection, LocalAccount account, byte[] wrapped, byte[] plain,
        byte[] key)
    {
        byte[] nonce = RandomNumberGenerator.GetBytes(12), tag = new byte[16], cipher = new byte[plain.Length];
        using (AesGcm aes = new(key, 16))
            aes.Encrypt(nonce, plain, cipher, tag, AssociatedData(format, protection, account));
        return new(format, protection, account, wrapped, nonce, tag, cipher);
    }

    private void Write(Record record, CancellationToken token, bool overwrite)
    {
        EnsurePrivateDirectory();
        string path = AccountPath(record.Account.Id), temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            byte[] bytes = JsonSerializer.SerializeToUtf8Bytes(record, AccountJsonContext.Default.Record);
            using (FileStream output = new(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096,
                       FileOptions.WriteThrough))
            {
                output.Write(bytes);
                output.Flush(true);
            }

            token.ThrowIfCancellationRequested();
            File.Move(temporary, path, overwrite);
        }
        finally
        {
            if (File.Exists(temporary)) File.Delete(temporary);
        }
    }

    private Record Read(Guid id)
    {
        string path = AccountPath(id);
        if (new FileInfo(path).Length < 1 || new FileInfo(path).Length > Math.Max(16384, maximumCredentialBytes * 2))
            throw new InvalidDataException("Account size bound.");
        Record record = JsonSerializer.Deserialize(File.ReadAllBytes(path), AccountJsonContext.Default.Record) ??
                        throw new InvalidDataException("Invalid account file.");
        LocalAccount? account = record.Account;
        if (record.Format is not (1 or 2 or 3) ||
            record.Protection is not (AccountProtection.Hello or AccountProtection.CurrentUser) ||
            (record.Format == 1 && record.Protection != AccountProtection.Hello) || account == null ||
            account.Id != id ||
            string.IsNullOrWhiteSpace(account.Label) || account.Label.Length > 32 ||
            account.Label.Any(char.IsControl) ||
            account.SavedUtc > time.GetUtcNow().AddMinutes(1) ||
            (record.Format == 1 ? account.ExpiresUtc != account.SavedUtc.AddHours(24) : account.ExpiresUtc != null) ||
            (record.Format == 3
                ? account.IdentityKey == null ||
                  !System.Text.RegularExpressions.Regex.IsMatch(account.IdentityKey, @"\A[0-9A-F]{64}\z")
                : account.IdentityKey != null) ||
            record.WrappedKey == null || record.WrappedKey.Length is < 1 or > 4096 ||
            (record.Protection == AccountProtection.Hello && record.WrappedKey.Length != 256) ||
            record.Nonce?.Length != 12 || record.Tag?.Length != 16 || record.Ciphertext == null ||
            record.Ciphertext.Length < 1 || record.Ciphertext.Length > maximumCredentialBytes)
            throw new InvalidDataException("Invalid local account contract.");
        _ = Select(record.Protection);
        return record;
    }

    private IAccountKeyProtector SelectDesired(bool useHello) => useHello
        ? Select(AccountProtection.Hello)
        : Select(AccountProtection.CurrentUser);

    private IAccountKeyProtector Select(string protection) => protector.Protection == protection ? protector :
        helloProtector?.Protection == protection ? helloProtector :
        throw new InvalidDataException("Account protection provider unavailable.");

    private static string ValidateLabel(string label)
    {
        label = label.Trim();
        return label.Length is < 1 or > 32 || label.Any(char.IsControl)
            ? throw new InvalidDataException("Invalid account label.")
            : label;
    }

    private string AccountPath(Guid id) => Path.Combine(root, id.ToString("N") + ".json");

    private static byte[] AssociatedData(int format, string protection, LocalAccount account) => Encoding.UTF8.GetBytes(
        $"TouchUILaunch.NTE|{format}|{protection}|{account.Id:N}|{account.Label}|{account.SavedUtc:O}|{account.ExpiresUtc:O}" +
        (format == 3 ? "|" + account.IdentityKey : ""));

    internal void EnsurePrivateDirectory()
    {
        if (!OperatingSystem.IsWindows())
        {
            Directory.CreateDirectory(root);
            return;
        }

        System.Security.AccessControl.DirectorySecurity acl = new();
        acl.SetAccessRuleProtection(true, false);
        var inheritance = System.Security.AccessControl.InheritanceFlags.ContainerInherit |
                          System.Security.AccessControl.InheritanceFlags.ObjectInherit;
        foreach (var sid in new[]
                 {
                     System.Security.Principal.WindowsIdentity.GetCurrent().User!,
                     new System.Security.Principal.SecurityIdentifier("S-1-5-18")
                 })
            acl.AddAccessRule(new(sid, System.Security.AccessControl.FileSystemRights.FullControl, inheritance,
                System.Security.AccessControl.PropagationFlags.None,
                System.Security.AccessControl.AccessControlType.Allow));
        DirectoryInfo info = new(root);
        if (!info.Exists) System.IO.FileSystemAclExtensions.Create(info, acl);
        else System.IO.FileSystemAclExtensions.SetAccessControl(info, acl);
    }

    [System.Text.Json.Serialization.JsonSerializable(typeof(Record))]
    private partial class AccountJsonContext : System.Text.Json.Serialization.JsonSerializerContext;
}
