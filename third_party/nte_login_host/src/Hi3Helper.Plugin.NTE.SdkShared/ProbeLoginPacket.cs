using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

// Encodes the fixed image's complete operation-2 packet. It does not send it.
// The extended layout is emitted at NTEGame RVA 0x47074A..0x470907; the SDK
// receiver consumes these fields at RVA 0x33320..0x33437.
internal sealed class ProbeLoginPacket : IDisposable
{
    internal const int Size = 0x594;
    private byte[] bytes;
    private bool disposed;
    private ProbeLoginPacket(byte[] encoded) => bytes = encoded;
    internal object Summary => new { operation = 2, bytes = disposed ? 0 : bytes.Length,
        encodedOnly = true, sentToGame = false };

    internal static ProbeLoginPacket Encode(ProbeIdentityEnvelope game, ProbeIdentityEnvelope channel,
        int platform, int channelId, JsonElement userInfo, int hostProcessId)
    {
        if (userInfo.ValueKind != JsonValueKind.Object) throw new InvalidDataException("Expected a structured user-information object.");
        byte[] information = Encoding.UTF8.GetBytes(userInfo.GetRawText());
        try { return Encode(game, channel, platform, channelId, information, hostProcessId); }
        finally { CryptographicOperations.ZeroMemory(information); }
    }

    internal static ProbeLoginPacket Encode(ProbeIdentityEnvelope game, ProbeIdentityEnvelope channel,
        int platform, int channelId, ReadOnlySpan<byte> information, int hostProcessId)
    {
        if (game.Stage != ProbeIdentityStage.Game || channel.Stage != ProbeIdentityStage.Channel ||
            !game.IdentityAvailable || !channel.IdentityAvailable)
            throw new InvalidOperationException("Both identity stages are required for packet encoding.");
        if (platform < 0 || channelId < 0 || hostProcessId <= 0)
            throw new ArgumentOutOfRangeException(nameof(platform), "Validated platform, channel and host identifiers are required.");
        (byte[] Uid, byte[] Token) gameFields = ([], []);
        (byte[] Uid, byte[] Token) channelFields = ([], []);
        byte[] encoded = new byte[Size];
        try
        {
            if (information.Length >= 1024) throw new InvalidDataException("User information exceeds the native capacity.");
            gameFields = game.CopyIdentity();
            channelFields = channel.CopyIdentity();
            BinaryPrimitives.WriteUInt32LittleEndian(encoded, 0x6B647331);
            BinaryPrimitives.WriteUInt32LittleEndian(encoded.AsSpan(4), 2);
            BinaryPrimitives.WriteUInt32LittleEndian(encoded.AsSpan(8), Size);
            gameFields.Uid.CopyTo(encoded, 0x0C);
            gameFields.Token.CopyTo(encoded, 0x4C);
            BinaryPrimitives.WriteInt32LittleEndian(encoded.AsSpan(0xCC), platform);
            BinaryPrimitives.WriteInt32LittleEndian(encoded.AsSpan(0xD0), channelId);
            channelFields.Uid.CopyTo(encoded, 0xD4);
            channelFields.Token.CopyTo(encoded, 0x114);
            information.CopyTo(encoded.AsSpan(0x194));
            // Header stays plain; the normal receiver decodes the body using
            // the process ID of its host window, not the client's PID.
            byte key = unchecked((byte)hostProcessId);
            for (int i = 12; i < encoded.Length; i++) encoded[i] ^= key;
            return new ProbeLoginPacket(encoded);
        }
        catch { CryptographicOperations.ZeroMemory(encoded); throw; }
        finally
        {
            CryptographicOperations.ZeroMemory(gameFields.Uid);
            CryptographicOperations.ZeroMemory(gameFields.Token);
            CryptographicOperations.ZeroMemory(channelFields.Uid);
            CryptographicOperations.ZeroMemory(channelFields.Token);
        }
    }

    internal byte[] CopyEncodedBytes()
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        return bytes.ToArray();
    }

    public void Dispose() { CryptographicOperations.ZeroMemory(bytes); bytes = []; disposed = true; }
    public override string ToString() => "SDK login packet [payload omitted]";
}
