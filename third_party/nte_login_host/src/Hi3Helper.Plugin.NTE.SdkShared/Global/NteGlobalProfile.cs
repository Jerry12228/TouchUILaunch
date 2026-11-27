using System.Net.Mail;
using System.Security.Cryptography;
using System.Text.Json;

namespace Nte.Login;

// Optional presentation only: never used for authentication, account matching,
// or SDK identity. The PC service must accept the provider exchange first.
internal sealed record NteGlobalProfile(string? Email = null, string? Name = null, string? MaskedPhone = null)
{
    internal static NteGlobalProfile FromService(JsonElement result) => new(
        EmailValue(Read(result, "email")), TextValue(Read(result, "username"), 80));
    internal static NteGlobalProfile FromEmail(string email) => new(EmailValue(email));
    internal static NteGlobalProfile FromPhone(string countryCode, string phone) => new(MaskedPhone: countryCode + " *** " + phone[^4..]);
    internal NteGlobalProfile Merge(NteGlobalProfile older) => new(Email ?? older.Email, Name ?? older.Name, MaskedPhone ?? older.MaskedPhone);

    internal static NteGlobalProfile AfterBrowserExchange(int provider, IReadOnlyDictionary<string, string> fields,
        string? googleProject)
    {
        fields.TryGetValue("thirdUsername", out string? name);
        if (provider == 1)
        {
            fields.TryGetValue("thirdEmail", out string? email);
            return new(EmailValue(email), TextValue(name, 80));
        }
        // This is a display hint returned through the official exchange, not a
        // second authentication verifier. Bind optional Google claims to the
        // official Firebase project, callback subject, issuer and lifetime.
        // A decoded JWT alone never creates a PC identity or saved account.
        string? googleEmail = null;
        byte[] header = [], payload = [];
        try
        {
            if (provider != 2 || googleProject is not { Length: > 0 and <= 32 }
                || googleProject.Any(c => !char.IsAsciiDigit(c))
                || !fields.TryGetValue("thirdId", out string? subject) || subject.Length is < 1 or > 256
                || !fields.TryGetValue("thirdAuthToken", out string? credential) || credential.Length > 16384)
                return new(null, TextValue(name, 80));
            string[] parts = credential.Split('.');
            if (parts.Length != 3 || parts[2].Length == 0) return new(null, TextValue(name, 80));
            header = Decode(parts[0]); payload = Decode(parts[1]);
            using JsonDocument head = JsonDocument.Parse(header), claims = JsonDocument.Parse(payload);
            JsonElement value = claims.RootElement;
            string? audience = Read(value, "aud");
            long now = DateTimeOffset.UtcNow.ToUnixTimeSeconds();
            if (Read(head.RootElement, "alg") == "RS256" && TextValue(Read(head.RootElement, "kid"), 128) != null
                && Read(value, "iss") is "https://accounts.google.com" or "accounts.google.com"
                && Read(value, "sub") == subject && audience != null
                && audience.StartsWith(googleProject + "-", StringComparison.Ordinal)
                && audience.EndsWith(".apps.googleusercontent.com", StringComparison.Ordinal)
                && value.TryGetProperty("exp", out JsonElement expires) && expires.TryGetInt64(out long exp) && exp > now - 60
                && value.TryGetProperty("iat", out JsonElement issued) && issued.TryGetInt64(out long iat) && iat <= now + 60
                && iat > now - 86400 && exp > iat && exp - iat <= 86400
                && value.TryGetProperty("email_verified", out JsonElement verified) && verified.ValueKind == JsonValueKind.True)
            {
                googleEmail = EmailValue(Read(value, "email"));
                name = TextValue(Read(value, "name"), 80) ?? name;
            }
        }
        catch (Exception error) when (error is FormatException or JsonException or InvalidOperationException) { }
        finally { CryptographicOperations.ZeroMemory(header); CryptographicOperations.ZeroMemory(payload); }
        return new(googleEmail, TextValue(name, 80));
    }

    private static byte[] Decode(string text)
    {
        if (text.Length is < 1 or > 12000 || text.Any(c => !char.IsAsciiLetterOrDigit(c) && c is not ('-' or '_')))
            throw new FormatException("Unsupported display claim encoding.");
        string encoded = text.Replace('-', '+').Replace('_', '/');
        return Convert.FromBase64String(encoded.PadRight((encoded.Length + 3) / 4 * 4, '='));
    }
    private static string? Read(JsonElement value, string name) => value.ValueKind == JsonValueKind.Object
        && value.TryGetProperty(name, out JsonElement item) && item.ValueKind == JsonValueKind.String ? item.GetString() : null;
    internal static string? TextValue(string? text, int limit)
    {
        string? value = text?.Trim();
        return value is { Length: > 0 } && value.Length <= limit && !value.Any(c => char.IsControl(c)
            || c is '\u202A' or '\u202B' or '\u202C' or '\u202D' or '\u202E' or '\u2066' or '\u2067' or '\u2068' or '\u2069') ? value : null;
    }
    private static string? EmailValue(string? text)
    {
        string? value = TextValue(text, 128);
        return value != null && MailAddress.TryCreate(value, out MailAddress? address) && address.Address == value ? value : null;
    }
    public override string ToString() => "Global display [personal data omitted]";
}
