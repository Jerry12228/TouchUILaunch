using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text.Json;

namespace Nte.Login;

internal static class NteGlobalPacket
{
    internal static byte[] EncodeWindowsPlatform(int hostProcessId)
    {
        if (hostProcessId <= 0) throw new ArgumentOutOfRangeException(nameof(hostProcessId));
        // The pinned official Global PC component sends 1520 with value 4.
        // Its SDK getter otherwise remains zero, even after successful login.
        byte[] packet = new byte[16];
        BinaryPrimitives.WriteUInt32LittleEndian(packet, 0x6B647331);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 1520);
        BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(8), packet.Length);
        BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(12), 4);
        for (int i = 12; i < packet.Length; i++) packet[i] ^= unchecked((byte)hostProcessId);
        return packet;
    }

    internal static byte[] Encode(uint command, int hostProcessId, Action<Utf8JsonWriter> write)
    {
        if (hostProcessId <= 0 || command is not (1538 or 1539 or 1540 or 1531 or 1546 or 1551))
            throw new ArgumentOutOfRangeException(nameof(command));
        using MemoryStream stream = new();
        try
        {
            using (Utf8JsonWriter json = new(stream)) write(json);
            if (stream.Length is <= 0 or > 48 * 1024) throw new InvalidDataException("Global SDK packet size bound.");
            byte[] packet = new byte[16 + stream.Length + 1];
            BinaryPrimitives.WriteUInt32LittleEndian(packet, 0x6B647331);
            BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), command);
            BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(8), packet.Length);
            BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(12), (int)stream.Length);
            stream.GetBuffer().AsSpan(0, (int)stream.Length).CopyTo(packet.AsSpan(16));
            for (int i = 12; i < packet.Length; i++) packet[i] ^= unchecked((byte)hostProcessId);
            return packet;
        }
        finally
        {
            CryptographicOperations.ZeroMemory(stream.GetBuffer());
        }
    }

    internal static byte[] EncodeQuery(NteGlobalSdkQuery request, JsonElement serviceResponse, int hostProcessId)
    {
        string result = NteGlobalSdkMetadata.Extract(request, serviceResponse);
        return Encode(request.ReplyCommand, hostProcessId, json =>
        {
            json.WriteStartObject();
            json.WriteNumber("contextId", request.ContextId);
            if (request.Notification == 113)
            {
                json.WriteNumber("code", 0);
                json.WriteString("message", "success");
            }

            json.WriteString("result", result);
            json.WriteEndObject();
        });
    }
}
