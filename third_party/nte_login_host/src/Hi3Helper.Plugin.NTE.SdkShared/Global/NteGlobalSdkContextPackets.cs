using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;

namespace Nte.Login;

// Public application/channel context observed before official account login.
// These fixed frames contain no user identity, credential or allow result.
internal static class NteGlobalSdkContextPackets
{
    internal static byte[][] Encode(int hostProcessId, NteOverseasRegion? region = null)
    {
        if (hostProcessId <= 0) throw new ArgumentOutOfRangeException(nameof(hostProcessId));
        byte[] application = Create(50, 236);
        try
        {
            region ??= NteOverseasRegion.Global;
            WriteAscii(application, 12, region.PackageName);
            WriteAscii(application, 0x4c, region.AppId);
            // The observed final optional field is empty; do not put the
            // authentication application key into an unrelated SDK slot.
            byte[] channel = Create(27, 656);
            WriteAscii(channel, 12, "4");
            WriteAscii(channel, 0x8c, "11");
            BinaryPrimitives.WriteInt32LittleEndian(channel.AsSpan(0x28c), 4);
            Xor(application, hostProcessId);
            Xor(channel, hostProcessId);
            return [application, channel];
        }
        catch
        {
            CryptographicOperations.ZeroMemory(application);
            throw;
        }
    }

    private static byte[] Create(uint command, int size)
    {
        byte[] packet = new byte[size];
        BinaryPrimitives.WriteUInt32LittleEndian(packet, 0x6B647331);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), command);
        BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(8), size);
        return packet;
    }

    private static void WriteAscii(byte[] packet, int offset, string value) =>
        Encoding.ASCII.GetBytes(value, packet.AsSpan(offset));

    private static void Xor(byte[] packet, int hostProcessId)
    {
        for (int i = 12; i < packet.Length; i++) packet[i] ^= unchecked((byte)hostProcessId);
    }
}
