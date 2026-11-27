using System.Buffers.Binary;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace Nte.LoginHost.Accounts;

internal sealed class SavedChannelCredential(ProbeIdentityEnvelope identity, string deviceId, int platform, int channelId) : IDisposable
{
    internal ProbeIdentityEnvelope Identity { get; } = identity;
    internal string DeviceId { get; } = deviceId;
    internal int Platform { get; } = platform;
    internal int ChannelId { get; } = channelId;
    public void Dispose() => Identity.Dispose();
}

internal static class StoredChannelIdentity
{
    private const int Magic = 0x3243544E, HeaderLength = 60;
    internal static string SubjectKey(byte[] credential, byte[] key)
    {
        using SavedChannelCredential saved = Decode(credential);
        (byte[] uid, byte[] token) = saved.Identity.CopyIdentity();
        int prefixLength = "NTE|1289|4|9|uid:"u8.Length;
        byte[] scoped = new byte[prefixLength + uid.Length];
        try
        {
            "NTE|1289|4|9|uid:"u8.CopyTo(scoped); uid.CopyTo(scoped, prefixLength);
            return Convert.ToHexString(HMACSHA256.HashData(key, scoped));
        }
        finally { CryptographicOperations.ZeroMemory(uid); CryptographicOperations.ZeroMemory(token); CryptographicOperations.ZeroMemory(scoped); }
    }
    internal static byte[] Encode(ProbeIdentityEnvelope channel, string deviceId, int platform, int channelId)
    {
        if (channel.Stage != ProbeIdentityStage.Channel || !channel.IdentityAvailable || platform != 4 || channelId != 9 ||
            !Regex.IsMatch(deviceId, @"\A[0-9a-f]{32}\z")) throw new InvalidDataException("Invalid saved channel context.");
        (byte[] uid, byte[] token) = channel.CopyIdentity();
        try
        {
            byte[] result = new byte[HeaderLength + uid.Length + token.Length];
            int[] fields = [Magic, 2, 1289, platform, channelId, uid.Length, token.Length];
            for (int i = 0; i < fields.Length; i++) BinaryPrimitives.WriteInt32LittleEndian(result.AsSpan(4 * i), fields[i]);
            Encoding.ASCII.GetBytes(deviceId, result.AsSpan(28, 32));
            uid.CopyTo(result, HeaderLength); token.CopyTo(result, HeaderLength + uid.Length); return result;
        }
        finally { CryptographicOperations.ZeroMemory(uid); CryptographicOperations.ZeroMemory(token); }
    }

    internal static SavedChannelCredential Decode(byte[] credential)
    {
        // Legacy payloads contain no owned device or channel scope. Keep them,
        // but require explicit authentication instead of guessing a device.
        if (credential.Length < HeaderLength + 2 || BinaryPrimitives.ReadInt32LittleEndian(credential) != Magic ||
            BinaryPrimitives.ReadInt32LittleEndian(credential.AsSpan(4)) != 2 || BinaryPrimitives.ReadInt32LittleEndian(credential.AsSpan(8)) != 1289 ||
            BinaryPrimitives.ReadInt32LittleEndian(credential.AsSpan(12)) != 4 || BinaryPrimitives.ReadInt32LittleEndian(credential.AsSpan(16)) != 9)
            throw new InvalidDataException("Saved channel requires authentication with the current format.");
        int uidLength = BinaryPrimitives.ReadInt32LittleEndian(credential.AsSpan(20)), tokenLength = BinaryPrimitives.ReadInt32LittleEndian(credential.AsSpan(24));
        string deviceId = Encoding.ASCII.GetString(credential, 28, 32);
        if (uidLength is < 1 or > 63 || tokenLength is < 1 or > 127 || credential.Length != HeaderLength + uidLength + tokenLength ||
            !Regex.IsMatch(deviceId, @"\A[0-9a-f]{32}\z")) throw new InvalidDataException("Invalid saved channel fields.");
        using MemoryStream output = new();
        try
        {
            using (Utf8JsonWriter writer = new(output))
            {
                writer.WriteStartObject(); writer.WriteNumber("code", 0); writer.WriteStartObject("result");
                writer.WriteString("userId", credential.AsSpan(HeaderLength, uidLength)); writer.WriteString("token", credential.AsSpan(HeaderLength + uidLength, tokenLength));
                writer.WriteEndObject(); writer.WriteEndObject(); writer.Flush();
            }
            ProbeIdentityEnvelope envelope = ProbeIdentityEnvelope.Parse(ProbeIdentityStage.Channel, 200, output.GetBuffer().AsSpan(0, (int)output.Length));
            if (!envelope.IdentityAvailable) { envelope.Dispose(); throw new InvalidDataException("Invalid stored channel identity."); }
            return new(envelope, deviceId, 4, 9);
        }
        finally { if (output.TryGetBuffer(out var buffer)) CryptographicOperations.ZeroMemory(buffer.AsSpan()); }
    }
}
