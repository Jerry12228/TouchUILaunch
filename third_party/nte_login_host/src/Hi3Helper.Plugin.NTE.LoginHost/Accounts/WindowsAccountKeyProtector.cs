using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;

namespace Nte.LoginHost.Accounts;

// Explicit default mode, selected by the launcher preference. It is never an
// error fallback for a failed Passport/Hello key operation.
internal sealed class WindowsAccountKeyProtector : IAccountKeyProtector
{
    public string Protection => AccountProtection.CurrentUser;
    public Task<byte[]> WrapAsync(Guid account, byte[] key, nint owner, CancellationToken token)
    { token.ThrowIfCancellationRequested(); return Task.FromResult(Transform(account, key, true)); }
    public Task<byte[]> UnwrapAsync(Guid account, byte[] wrappedKey, nint owner, CancellationToken token)
    { token.ThrowIfCancellationRequested(); return Task.FromResult(Transform(account, wrappedKey, false)); }
    public void Delete(Guid account) { /* DPAPI owns no per-account persistent key. */ }

    private static byte[] Transform(Guid account, byte[] input, bool protect)
    {
        byte[] entropy = Encoding.UTF8.GetBytes("TouchUILaunch.NTE.Account.Key.v2|" + account.ToString("N"));
        DataBlob source = Allocate(input), extra = Allocate(entropy), result = default;
        nint description = 0;
        try
        {
            bool success = protect ? CryptProtectData(ref source, "TouchUILaunch NTE saved account key", ref extra, 0, 0, 1, out result)
                : CryptUnprotectData(ref source, out description, ref extra, 0, 0, 1, out result);
            if (!success) throw new CryptographicException("Current-user account key protection failed.", new Win32Exception(Marshal.GetLastWin32Error()));
            if (result.Length is < 1 or > 4096) throw new CryptographicException("Protected account key size bound.");
            byte[] bytes = new byte[result.Length]; Marshal.Copy(result.Data, bytes, 0, bytes.Length); return bytes;
        }
        finally
        {
            CryptographicOperations.ZeroMemory(entropy); Clear(source, false); Clear(extra, false); Clear(result, true);
            if (description != 0) LocalFree(description);
        }
    }
    private static DataBlob Allocate(byte[] bytes)
    { nint pointer = Marshal.AllocHGlobal(bytes.Length); Marshal.Copy(bytes, 0, pointer, bytes.Length); return new() { Length = bytes.Length, Data = pointer }; }
    private static void Clear(DataBlob blob, bool local)
    {
        if (blob.Data == 0) return;
        if (blob.Length is > 0 and <= 65536) Marshal.Copy(new byte[blob.Length], 0, blob.Data, blob.Length);
        if (local) LocalFree(blob.Data); else Marshal.FreeHGlobal(blob.Data);
    }
    [StructLayout(LayoutKind.Sequential)] private struct DataBlob { public int Length; public nint Data; }
    [DllImport("crypt32", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
    private static extern bool CryptProtectData(ref DataBlob input, string description, ref DataBlob entropy, nint reserved, nint prompt, uint flags, out DataBlob output);
    [DllImport("crypt32", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
    private static extern bool CryptUnprotectData(ref DataBlob input, out nint description, ref DataBlob entropy, nint reserved, nint prompt, uint flags, out DataBlob output);
    [DllImport("kernel32")] private static extern nint LocalFree(nint pointer);
}
