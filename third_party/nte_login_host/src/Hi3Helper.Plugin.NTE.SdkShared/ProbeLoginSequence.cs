using System.Buffers.Binary;
using System.Security.Cryptography;

// Current bridge version 2.0.8.0 takes the extended-info branch. The official
// producer sends operation 0x40D before operation 2. This object only encodes;
// it neither authenticates an account nor transmits a successful SDK reply.
internal sealed class ProbeLoginSequence : IDisposable
{
    private byte[] extraPacket;
    private readonly ProbeLoginPacket loginPacket;
    private bool disposed;

    private ProbeLoginSequence(byte[] extra, ProbeLoginPacket login)
    {
        extraPacket = extra; loginPacket = login;
    }

    internal object Summary => new { encodedOnly = true, sentToGame = false,
        operations = new[] { 0x40D, 2 }, bytes = disposed ? 0 : extraPacket.Length + ProbeLoginPacket.Size };

    internal static ProbeLoginSequence EncodeCurrentBridge(ProbeIdentityEnvelope game, ProbeIdentityEnvelope channel,
        int platform, int channelId, int hostProcessId)
    {
        if (hostProcessId <= 0) throw new ArgumentOutOfRangeException(nameof(hostProcessId));
        byte[] information = game.CopyExtraInformation();
        byte[] extra = new byte[information.Length + 17];
        try
        {
            if (extra.Length > 65536) throw new InvalidDataException("Extended information exceeds the protocol capacity.");
            BinaryPrimitives.WriteUInt32LittleEndian(extra, 0x6B647331);
            BinaryPrimitives.WriteUInt32LittleEndian(extra.AsSpan(4), 0x40D);
            BinaryPrimitives.WriteInt32LittleEndian(extra.AsSpan(8), extra.Length);
            BinaryPrimitives.WriteInt32LittleEndian(extra.AsSpan(12), information.Length);
            information.CopyTo(extra, 16);
            byte key = unchecked((byte)hostProcessId);
            for (int i = 12; i < extra.Length; i++) extra[i] ^= key;
            // strcpy_s clears the legacy destination on overflow. The complete
            // object remains available through the preceding extended packet.
            ReadOnlySpan<byte> legacy = information.Length < 1024 ? information : [];
            ProbeLoginPacket login = ProbeLoginPacket.Encode(game, channel, platform, channelId, legacy, hostProcessId);
            return new ProbeLoginSequence(extra, login);
        }
        catch { CryptographicOperations.ZeroMemory(extra); throw; }
        finally { CryptographicOperations.ZeroMemory(information); }
    }

    internal byte[][] CopyEncodedPackets()
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        return [extraPacket.ToArray(), loginPacket.CopyEncodedBytes()];
    }

    public void Dispose()
    {
        CryptographicOperations.ZeroMemory(extraPacket);
        extraPacket = []; loginPacket.Dispose(); disposed = true;
    }

    public override string ToString() => "SDK login sequence [payloads omitted]";
}
