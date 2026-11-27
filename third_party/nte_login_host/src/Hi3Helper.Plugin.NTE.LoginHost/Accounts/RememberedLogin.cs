using System.Buffers.Binary;
using System.Security.Cryptography;

namespace Nte.LoginHost.Accounts;

// Stores only the local record ID. The credential remains in its account file
// and still passes the configured protection and official service checks.
internal sealed class RememberedLogin(string path, IAccountKeyProtector protector)
{
    private static readonly Guid StateId = new("573ad718-42bb-4459-94dd-4e409b357d04");
    internal bool HasState => File.Exists(path);
    internal static LocalAccount? Resolve(Guid? last, bool hasState, IReadOnlyList<LocalAccount> accounts) =>
        accounts.FirstOrDefault(a => a.Id == last) ?? (!hasState && accounts.Count == 1 ? accounts[0] : null);
    internal async Task<Guid?> ReadAsync(CancellationToken token)
    {
        if (!File.Exists(path)) return null;
        if (new FileInfo(path).Length is < 1 or > 4096) throw new InvalidDataException("Invalid remembered login state.");
        byte[] plain = await protector.UnwrapAsync(StateId, File.ReadAllBytes(path), 0, token).ConfigureAwait(false);
        try
        {
            token.ThrowIfCancellationRequested();
            if (plain.Length != 32 || BinaryPrimitives.ReadInt32LittleEndian(plain) != 1) throw new InvalidDataException("Unknown remembered login format.");
            Guid id = new(plain.AsSpan(4, 16));
            return id == Guid.Empty ? null : id;
        }
        finally { CryptographicOperations.ZeroMemory(plain); }
    }

    internal async Task SetAsync(Guid? account, CancellationToken token)
    {
        byte[] plain = new byte[32];
        try
        {
            BinaryPrimitives.WriteInt32LittleEndian(plain, 1); (account ?? Guid.Empty).TryWriteBytes(plain.AsSpan(4, 16));
            byte[] wrapped = await protector.WrapAsync(StateId, plain, 0, token).ConfigureAwait(false);
            Directory.CreateDirectory(Path.GetDirectoryName(Path.GetFullPath(path))!);
            string temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try { File.WriteAllBytes(temporary, wrapped); token.ThrowIfCancellationRequested(); File.Move(temporary, path, true); }
            finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }
        finally { CryptographicOperations.ZeroMemory(plain); }
    }
}
