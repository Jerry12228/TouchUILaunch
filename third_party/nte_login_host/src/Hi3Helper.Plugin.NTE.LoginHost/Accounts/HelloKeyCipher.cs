using Microsoft.Win32.SafeHandles;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Security.Cryptography;

namespace Nte.LoginHost.Accounts;

[SupportedOSPlatform("windows")]
internal static class HelloKeyCipher
{
    internal static byte[] Encrypt(CngKey key, byte[] plain) => Transform(key, plain, false);
    internal static byte[] Decrypt(CngKey key, byte[] cipher) => Transform(key, cipher, true);

    private static byte[] Transform(CngKey key, byte[] input, bool decrypt)
    {
        if (input.Length != (decrypt ? 256 : 32)) throw new CryptographicException("Invalid account key block size.");
        // One native decrypt call avoids a second verification from a managed
        // size-query pass. Never export the Passport key or use another KSP.
        byte[] output = new byte[256];
        try
        {
            using SafeNCryptKeyHandle handle = key.Handle;
            int status, length;
            if (decrypt) status = NCryptDecrypt(handle, input, input.Length, 0, output, output.Length, out length, 2);
            else status = NCryptEncrypt(handle, input, input.Length, 0, output, output.Length, out length, 2);
            if (status != 0) throw new CryptographicException(status);
            if (length != (decrypt ? 32 : 256)) throw new CryptographicException("Unexpected account key block size.");
            return output.AsSpan(0, length).ToArray();
        }
        finally { CryptographicOperations.ZeroMemory(output); }
    }

    [DllImport("ncrypt.dll")]
    private static extern int NCryptEncrypt(SafeNCryptKeyHandle key, byte[] input, int inputLength,
        nint padding, byte[] output, int outputLength, out int resultLength, int flags);
    [DllImport("ncrypt.dll")]
    private static extern int NCryptDecrypt(SafeNCryptKeyHandle key, byte[] input, int inputLength,
        nint padding, byte[] output, int outputLength, out int resultLength, int flags);
}
