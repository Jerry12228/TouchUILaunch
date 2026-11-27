using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Nte.Login;

// CHDGamePlayerMgr::onGlobalUserInfoGetted normalizes the root and linked
// account UIDs before caching the compact JSON returned by global_GetUserInfo.
// Keep the official service identity intact for subsequent token validation.
internal static class NteGlobalUserInfo
{
    internal static string Encode(NteGlobalIdentity identity)
    {
        using MemoryStream stream = new();
        try
        {
            using (Utf8JsonWriter json = new(stream)) WriteAccount(json, identity.Result, linked: false);
            return Encoding.UTF8.GetString(stream.GetBuffer(), 0, checked((int)stream.Length));
        }
        finally { CryptographicOperations.ZeroMemory(stream.GetBuffer()); }
    }

    private static void WriteAccount(Utf8JsonWriter json, JsonElement account, bool linked)
    {
        json.WriteStartObject();
        foreach (JsonProperty field in account.EnumerateObject())
        {
            if (field.Name == "uid") json.WriteString(field.Name, NteGlobalIdentity.ReadUserId(field.Value));
            else if (!linked && field.Name == "thirdUsers" && field.Value.ValueKind == JsonValueKind.Array)
            {
                json.WriteStartArray(field.Name);
                foreach (JsonElement item in field.Value.EnumerateArray())
                    if (item.ValueKind == JsonValueKind.Object) WriteAccount(json, item, linked: true);
                    else item.WriteTo(json);
                json.WriteEndArray();
            }
            else field.WriteTo(json);
        }
        json.WriteEndObject();
    }
}
