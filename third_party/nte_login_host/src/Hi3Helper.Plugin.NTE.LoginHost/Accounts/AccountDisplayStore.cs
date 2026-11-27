using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Nte.LoginHost.Accounts;

// Display data is deliberately separate from sign-in credentials. Listing an
// account may decrypt this current-user-only sidecar, but never a Hello key.
internal sealed record AccountDisplayProfile(
    int Provider,
    string? Email = null,
    string? Name = null,
    string? MaskedPhone = null,
    string? InitialLabel = null)
{
    public override string ToString() => "Account display [personal data omitted]";
}

internal sealed partial class AccountDisplayStore(string directory, IAccountKeyProtector protector)
{
    private readonly string root = Path.GetFullPath(directory);

    private sealed record Document(int Format, Guid Account, string? IdentityKey, AccountDisplayProfile Profile);

    internal async Task<AccountDisplayProfile?> ReadAsync(LocalAccount account, CancellationToken token)
    {
        string path = PathFor(account.Id);
        if (!File.Exists(path)) return null;
        byte[] plain = [];
        try
        {
            if (new FileInfo(path).Length is < 1 or > 4096) throw new InvalidDataException("Display size bound.");
            plain = await protector.UnwrapAsync(Purpose(account.Id), File.ReadAllBytes(path), 0, token);
            Document value = JsonSerializer.Deserialize(plain, DisplayJsonContext.Default.Document) ??
                             throw new InvalidDataException("Invalid display data.");
            if (value.Format != 1 || value.Account != account.Id || value.IdentityKey != account.IdentityKey)
                throw new InvalidDataException("Display account mismatch.");
            Validate(value.Profile);
            return value.Profile;
        }
        finally
        {
            CryptographicOperations.ZeroMemory(plain);
        }
    }

    internal async Task<PendingAccountSave> AttachAsync(PendingAccountSave credentialChange,
        AccountDisplayProfile incoming, CancellationToken token)
    {
        LocalAccount account = credentialChange.Account;
        byte[]? previous = null;
        bool written = false;
        try
        {
            AccountDisplayProfile? old;
            try
            {
                old = await ReadAsync(account, token);
            }
            catch (Exception error) when (IsDisplayError(error))
            {
                old = null;
            }

            string path = PathFor(account.Id);
            if (File.Exists(path))
            {
                if (new FileInfo(path).Length > 4096) throw new InvalidDataException("Display size bound.");
                previous = File.ReadAllBytes(path);
            }

            bool sameProvider = old?.Provider == incoming.Provider;
            AccountDisplayProfile merged = incoming with
            {
                Email = incoming.Email ?? (sameProvider ? old?.Email : null),
                Name = incoming.Name ?? (sameProvider ? old?.Name : null),
                MaskedPhone = incoming.MaskedPhone ?? (sameProvider ? old?.MaskedPhone : null),
                InitialLabel = old?.InitialLabel ?? incoming.InitialLabel
            };
            Validate(merged);
            byte[] plain =
                JsonSerializer.SerializeToUtf8Bytes(new Document(1, account.Id, account.IdentityKey, merged),
                    DisplayJsonContext.Default.Document);
            byte[] wrapped;
            try
            {
                wrapped = await protector.WrapAsync(Purpose(account.Id), plain, 0, token);
            }
            finally
            {
                CryptographicOperations.ZeroMemory(plain);
            }

            Write(account.Id, wrapped, token);
            written = true;
            return new(account, () =>
            {
                try
                {
                    if (previous == null) Remove(account.Id);
                    else Write(account.Id, previous, CancellationToken.None);
                }
                finally
                {
                    credentialChange.Dispose();
                }
            }, credentialChange.Commit);
        }
        finally
        {
            if (!written) credentialChange.Dispose();
        }
    }

    internal void Remove(Guid id)
    {
        string path = PathFor(id);
        if (File.Exists(path)) File.Delete(path);
    }

    internal static bool IsDisplayError(Exception error) => error is IOException or UnauthorizedAccessException
        or CryptographicException or JsonException;

    private static void Validate(AccountDisplayProfile value)
    {
        if (value.Provider is not (0 or 1 or 2 or 7 or 8) || !Valid(value.Email, 128) || !Valid(value.Name, 80)
            || !Valid(value.InitialLabel, 32) || !Valid(value.MaskedPhone, 32)
            || (value.MaskedPhone != null &&
                !System.Text.RegularExpressions.Regex.IsMatch(value.MaskedPhone, value.Provider == 7
                    ? @"\A\+[1-9][0-9]{0,3} \*{3} [0-9]{4}\z"
                    : @"\A1[0-9]{2}\*{4}[0-9]{4}\z")))
            throw new InvalidDataException("Invalid account display contract.");
    }

    private static bool Valid(string? value, int limit) => value == null || value.Length is > 0 && value.Length <= limit
        && !value.Any(c => char.IsControl(c) || c is '\u202A' or '\u202B' or '\u202C' or '\u202D' or '\u202E'
            or '\u2066' or '\u2067' or '\u2068' or '\u2069');

    private string PathFor(Guid id) => Path.Combine(root, id.ToString("N") + ".display");

    private static Guid Purpose(Guid id) => new(SHA256
        .HashData(Encoding.ASCII.GetBytes("TouchUILaunch.NTE.Display.v1|" + id.ToString("N"))).AsSpan(0, 16));

    private void Write(Guid id, byte[] value, CancellationToken token)
    {
        if (value.Length is < 1 or > 4096) throw new InvalidDataException("Display size bound.");
        string path = PathFor(id), temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            using (FileStream file = new(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096,
                       FileOptions.WriteThrough))
            {
                file.Write(value);
                file.Flush(true);
            }

            token.ThrowIfCancellationRequested();
            File.Move(temporary, path, true);
        }
        finally
        {
            if (File.Exists(temporary)) File.Delete(temporary);
        }
    }

    [System.Text.Json.Serialization.JsonSerializable(typeof(Document))]
    private partial class DisplayJsonContext : System.Text.Json.Serialization.JsonSerializerContext;
}
