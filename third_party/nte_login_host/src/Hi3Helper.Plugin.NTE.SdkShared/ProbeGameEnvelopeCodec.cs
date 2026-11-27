using System.Security.Cryptography;
using System.Text;

// Fixed OneLogin wrapper 0x47C480: compact UTF-8 JSON -> AES-128-ECB/PKCS7
// -> Base64. The HTTP body is that Base64 directly, not form fields. Its reply
// is decoded using the same application key before inspecting JSON.
internal sealed class ProbeGameEnvelopeCodec : IDisposable
{
    private readonly byte[] key;
    private bool disposed;
    internal ProbeGameEnvelopeCodec(ReadOnlySpan<byte> applicationKeyTail)
    {
        if (applicationKeyTail.Length != 16) throw new InvalidDataException("A validated application encryption key is required.");
        key = applicationKeyTail.ToArray();
    }

    internal byte[] Encode(ReadOnlySpan<byte> json)
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        if (json.Length is 0 or > 512 * 1024) throw new InvalidDataException("Game envelope size bound.");
        using Aes aes = Aes.Create(); aes.Key = key;
        byte[] encrypted = aes.EncryptEcb(json, PaddingMode.PKCS7);
        try { return Encoding.ASCII.GetBytes(Convert.ToBase64String(encrypted)); }
        finally { CryptographicOperations.ZeroMemory(encrypted); }
    }

    internal byte[] Decode(ReadOnlySpan<byte> encoded)
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        if (encoded.Length is 0 or > 1024 * 1024 || encoded.ContainsAnyExceptInRange((byte)0x09, (byte)0x7E))
            throw new InvalidDataException("Invalid encoded game envelope.");
        byte[] encrypted = Convert.FromBase64String(Encoding.ASCII.GetString(encoded));
        try
        {
            if (encrypted.Length == 0 || encrypted.Length % 16 != 0) throw new InvalidDataException("Invalid game cipher length.");
            using Aes aes = Aes.Create(); aes.Key = key;
            return aes.DecryptEcb(encrypted, PaddingMode.PKCS7);
        }
        finally { CryptographicOperations.ZeroMemory(encrypted); }
    }

    public void Dispose() { CryptographicOperations.ZeroMemory(key); disposed = true; }
    internal byte[] CopyKey() { ObjectDisposedException.ThrowIf(disposed, this); return key.ToArray(); }
    public override string ToString() => "Game envelope codec [key omitted]";
}
