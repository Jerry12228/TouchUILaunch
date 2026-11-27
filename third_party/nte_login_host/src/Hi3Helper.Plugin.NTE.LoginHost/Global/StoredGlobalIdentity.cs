using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using Nte.Login;

namespace Nte.LoginHost;

// Only PC uid/token/provider are retained. A decoded record must pass the
// official token-login endpoint before being delivered to a new game.
internal static class StoredGlobalIdentity
{
    internal static byte[] Encode(NteGlobalIdentity identity)
    {
        using MemoryStream stream = new();
        try
        {
            using (Utf8JsonWriter json = new(stream))
            {
                json.WriteStartObject(); json.WriteNumber("format", 1); json.WriteNumber("appId", identity.Region.ApplicationId);
                json.WriteString("area", identity.Region.Area); json.WriteNumber("provider", identity.Provider);
                json.WriteString("uid", NteGlobalIdentity.ReadUserId(identity.Result.GetProperty("uid")));
                json.WriteString("token", identity.Result.GetProperty("token").GetString()); json.WriteEndObject();
            }
            if (stream.Length > 12288) throw new InvalidDataException("Saved Global identity size bound.");
            return stream.ToArray();
        }
        finally { CryptographicOperations.ZeroMemory(stream.GetBuffer()); }
    }

    internal static NteGlobalIdentity Decode(byte[] credential, NteOverseasRegion? region = null)
    {
        region ??= NteOverseasRegion.Global;
        if (credential.Length is < 1 or > 12288) throw new InvalidDataException("Saved Global identity size bound.");
        using JsonDocument json = JsonDocument.Parse(credential);
        JsonElement root = json.RootElement;
        if (root.GetProperty("format").GetInt32() != 1 || root.GetProperty("appId").GetInt32() != region.ApplicationId ||
            root.GetProperty("area").GetString() != region.Area) throw new InvalidDataException("Saved Global region mismatch.");
        using MemoryStream stream = new();
        try
        {
            using (Utf8JsonWriter writer = new(stream))
            {
                writer.WriteStartObject(); writer.WriteNumber("code", 0); writer.WriteStartObject("result");
                root.GetProperty("uid").WriteToProperty(writer, "uid");
                root.GetProperty("token").WriteToProperty(writer, "token");
                writer.WriteEndObject(); writer.WriteEndObject();
            }
            using JsonDocument response = JsonDocument.Parse(stream.GetBuffer().AsMemory(0, (int)stream.Length));
            return NteGlobalIdentity.FromService(response.RootElement, root.GetProperty("provider").GetInt32(), region);
        }
        finally { CryptographicOperations.ZeroMemory(stream.GetBuffer()); }
    }

    internal static string SubjectKey(byte[] credential, byte[] key) => SubjectKey(credential, key, NteOverseasRegion.Global);

    internal static string SubjectKey(byte[] credential, byte[] key, NteOverseasRegion? region)
    {
        region ??= NteOverseasRegion.Global;
        using NteGlobalIdentity identity = Decode(credential, region);
        byte[] scoped = Encoding.UTF8.GetBytes("NTE|" + region.Name + "|" + region.AppId + "|" + region.Area + "|uid:" +
            NteGlobalIdentity.ReadUserId(identity.Result.GetProperty("uid")));
        try { return Convert.ToHexString(HMACSHA256.HashData(key, scoped)); }
        finally { CryptographicOperations.ZeroMemory(scoped); }
    }

    private static void WriteToProperty(this JsonElement value, Utf8JsonWriter writer, string name)
    { writer.WritePropertyName(name); value.WriteTo(writer); }
}
