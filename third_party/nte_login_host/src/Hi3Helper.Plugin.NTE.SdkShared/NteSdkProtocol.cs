using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;

namespace Nte.Login;

// Select this contract from the verified installed SDK, once per launch. All
// registration consumers share its validated result instead of selecting their
// own version or falling back to a default.
internal sealed class NteSdkProtocol
{
    private const string Version208 = "2.0.8.0", Version209 = "2.0.9.0";
    private const string CnApplication = NteRegionDefinition.CnAppId;
    internal static NteSdkProtocol Cn208 { get; } = new(CnApplication, Version208);
    internal static NteSdkProtocol Cn209 { get; } = new(CnApplication, Version209);
    internal static NteSdkProtocol Overseas208 { get; } = new("global", Version208);
    internal static NteSdkProtocol Overseas209 { get; } = new("global", Version209);

    internal string Application { get; }
    internal string Version { get; }
    internal bool IsCn => Application == CnApplication;

    private NteSdkProtocol(string application, string version)
    { Application = application; Version = version; }

    internal NteSdkRegistration ReadRegistration(ReadOnlySpan<byte> frame, int clientId)
    {
        if (clientId <= 0 || frame.Length != 1244 || BinaryPrimitives.ReadUInt32LittleEndian(frame) != 0x6B647331 ||
            BinaryPrimitives.ReadUInt32LittleEndian(frame[4..]) != 0 || BinaryPrimitives.ReadInt32LittleEndian(frame[8..]) != frame.Length)
            throw new InvalidDataException("Expected the owned client's SDK registration frame.");
        byte[] body = frame[12..].ToArray();
        try
        {
            for (int i = 0; i < body.Length; i++) body[i] ^= unchecked((byte)clientId);
            if (ReadField(body, 4, 256) != Application || ReadField(body, 0x40C, 64) != Version)
                throw new InvalidDataException("Registration does not match the selected SDK protocol.");
            return new(this, clientId, body);
        }
        catch { CryptographicOperations.ZeroMemory(body); throw; }
    }

    internal static string ReadField(ReadOnlySpan<byte> body, int offset, int bytes)
    {
        string value = Encoding.Unicode.GetString(body.Slice(offset, bytes));
        int end = value.IndexOf('\0');
        return end < 0 ? value : value[..end];
    }
}

// Contains application key material. Its owner keeps it only for registration
// initialization and disposes it before processing any login requests.
internal sealed class NteSdkRegistration(NteSdkProtocol protocol, int clientId, byte[] body) : IDisposable
{
    internal const string CnConfigurationName = "oneSdkConfig.config";
    private bool disposed;
    internal NteSdkProtocol Protocol { get; } = protocol;
    internal int ClientId { get; } = clientId;
    private ReadOnlySpan<byte> Body
    {
        get { ObjectDisposedException.ThrowIf(disposed, this); return body; }
    }
    // HWND is a four-byte protocol field even in the x64 bridge.
    internal nint CallbackWindow => (nint)BinaryPrimitives.ReadUInt32LittleEndian(Body);

    internal byte[] ReadCnApplicationKey()
    {
        if (!Protocol.IsCn ||
            NteSdkProtocol.ReadField(Body, 0x204, 520) != CnConfigurationName ||
            BinaryPrimitives.ReadUInt16LittleEndian(Body[(0x104 + 64)..]) != 0)
            throw new InvalidDataException("Unverified CN registration metadata.");
        byte[] key = new byte[32];
        try
        {
            for (int i = 0; i < key.Length; i++)
            {
                ushort character = BinaryPrimitives.ReadUInt16LittleEndian(Body[(0x104 + 2 * i)..]);
                if (character is < 32 or > 126) throw new InvalidDataException("Unverified application key encoding.");
                key[i] = (byte)character;
            }
            return key;
        }
        catch { CryptographicOperations.ZeroMemory(key); throw; }
    }

    public void Dispose()
    {
        if (disposed) return;
        disposed = true;
        CryptographicOperations.ZeroMemory(body);
    }
}
