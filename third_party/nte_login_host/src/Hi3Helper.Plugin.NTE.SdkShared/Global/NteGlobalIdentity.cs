using System.Globalization;
using System.Net;
using System.Net.Mail;
using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Nte.Login;

internal sealed class NteGlobalIdentity : IDisposable
{
    private readonly JsonDocument document;
    internal JsonElement Result => document.RootElement;
    internal int Provider { get; }
    internal NteOverseasRegion Region { get; }

    internal NteGlobalProfile Profile { get; private set; }

    // The selected browser provider and the service's SDK login type are
    // separate values. QHDGlobalAuthManager forwards result.loginType.
    internal int LoginType { get; }

    private NteGlobalIdentity(JsonElement result, int provider, NteOverseasRegion region)
    {
        Region = region;
        Profile = NteGlobalProfile.FromService(result);
        LoginType = result.TryGetProperty("loginType", out JsonElement type)
            ? type.ValueKind == JsonValueKind.Number && type.TryGetInt32(out int value) && value >= 0
                ? value
                : throw new InvalidDataException("Unsupported global SDK login type.")
            : 0; // QJsonValue::toInt() uses zero for a missing property.
        using MemoryStream stream = new();
        using (Utf8JsonWriter json = new(stream))
        {
            json.WriteStartObject();
            foreach (JsonProperty field in result.EnumerateObject()) field.WriteTo(json);
            json.WriteEndObject();
        }

        Utf8JsonReader reader = new(stream.GetBuffer().AsSpan(0, (int)stream.Length));
        try
        {
            document = JsonDocument.ParseValue(ref reader);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(stream.GetBuffer());
        }

        Provider = provider;
    }

    internal static NteGlobalIdentity FromService(JsonElement response, int provider, NteOverseasRegion? region = null)
    {
        JsonElement result = NteGlobalAuthentication.RequireResult(response);
        if ((provider is not (1 or 2 or 8) && !(provider == 7 && region?.HostRegion == NteHostRegion.Taiwan)) || !result.TryGetProperty("uid", out JsonElement uid) ||
            ReadUserId(uid) is not { Length: > 0 and <= 256 } ||
            !result.TryGetProperty("token", out JsonElement token) ||
            token.ValueKind != JsonValueKind.String || token.GetString() is not { Length: > 0 and <= 8192 })
            throw new InvalidDataException("Incomplete official global identity.");
        return new(result, provider, region ?? NteOverseasRegion.Global);
    }

    internal static string ReadUserId(JsonElement value) => value.ValueKind switch
    {
        JsonValueKind.String => value.GetString() ?? "",
        JsonValueKind.Number when value.TryGetUInt64(out ulong id) && id > 0 => id.ToString(
            CultureInfo.InvariantCulture),
        _ => throw new InvalidDataException("Unsupported global user identifier.")
    };

    internal NteGlobalIdentity WithProfile(NteGlobalProfile profile)
    {
        Profile = profile.Merge(Profile);
        return this;
    }

    internal NteGlobalIdentity Clone() => new(Result, Provider, Region) { Profile = Profile };
    public void Dispose() => document.Dispose();
    public override string ToString() => "Global identity [payload omitted]";
}
