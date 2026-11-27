using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

internal enum ProbeIdentityStage { Channel, Game }

// Structural response parsing only. IdentityAvailable does not mean that the
// game accepted a session. Credentials never enter the diagnostic summary.
internal sealed class ProbeIdentityEnvelope : IDisposable
{
    private byte[] uid = [];
    private byte[] token = [];
    private byte[] extraInformation = [];
    private bool disposed;
    private static readonly UTF8Encoding StrictUtf8 = new(false, true);

    private ProbeIdentityEnvelope(ProbeIdentityStage stage, int httpStatus)
    {
        Stage = stage;
        HttpStatus = httpStatus;
    }

    internal ProbeIdentityStage Stage { get; }
    internal int HttpStatus { get; }
    internal int? ServerCode { get; private set; }
    internal bool IdentityAvailable => !disposed && uid.Length != 0 && token.Length != 0;
    internal string Disposition { get; private set; } = "invalid-response";
    internal object Summary => new { stage = Stage.ToString(), httpStatus = HttpStatus,
        serverCode = ServerCode, identityAvailable = IdentityAvailable, disposition = Disposition,
        extraInformationBytes = extraInformation.Length, gameSessionAccepted = false };

    internal static ProbeIdentityEnvelope Parse(ProbeIdentityStage stage, int httpStatus, ReadOnlySpan<byte> bytes)
    {
        ProbeIdentityEnvelope envelope = new(stage, httpStatus);
        if (httpStatus is < 200 or >= 300)
        {
            envelope.Disposition = "http-failure";
            return envelope;
        }
        if (bytes.Length is 0 or > 1024 * 1024) return envelope;
        byte[] owned = bytes.ToArray();
        byte[] parsedInformation = [];
        try
        {
            using JsonDocument json = JsonDocument.Parse(owned, new JsonDocumentOptions { MaxDepth = 32 });
            JsonElement root = json.RootElement;
            if (root.ValueKind != JsonValueKind.Object || !root.TryGetProperty("code", out JsonElement code) ||
                code.ValueKind != JsonValueKind.Number || !code.TryGetInt32(out int value)) return envelope;
            envelope.ServerCode = value;
            if (value != 0)
            {
                envelope.Disposition = "server-rejected";
                return envelope;
            }
            JsonElement identity;
            string uidName;
            if (stage == ProbeIdentityStage.Channel)
            {
                if (!root.TryGetProperty("result", out identity) || identity.ValueKind != JsonValueKind.Object) return envelope;
                uidName = "userId";
            }
            else { identity = root; uidName = "uid"; }
            // Reject duplicates of fields used to accept or transfer identity.
            // JsonElement.GetProperty otherwise accepts the last duplicate.
            if (Count(root, "code") != 1 || (stage == ProbeIdentityStage.Channel && Count(root, "result") != 1) ||
                Count(identity, uidName) != 1 || Count(identity, "token") != 1) return envelope;
            if (stage == ProbeIdentityStage.Game && root.TryGetProperty("extraOne", out JsonElement extra))
            {
                if (Count(root, "extraOne") != 1 || extra.ValueKind != JsonValueKind.Object) return envelope;
                parsedInformation = JsonSerializer.SerializeToUtf8Bytes(extra, NteJsonContext.Relaxed.JsonElement);
                // Operation 0x40D has a 16-byte header and a trailing NUL.
                if (parsedInformation.Length > 65536 - 17) return envelope;
            }
            if (!identity.TryGetProperty(uidName, out JsonElement id) || !identity.TryGetProperty("token", out JsonElement secret)) return envelope;
            string? idText = id.ValueKind switch
            {
                JsonValueKind.String => id.GetString(),
                JsonValueKind.Number when id.TryGetUInt64(out ulong number) && number != 0 => number.ToString(System.Globalization.CultureInfo.InvariantCulture),
                _ => null
            };
            string? tokenText = secret.ValueKind == JsonValueKind.String ? secret.GetString() : null;
            // Native operation 2 uses byte buffers of 64 and 128 bytes including
            // terminators, rather than the registration frame's wide strings.
            byte[] parsedUid = EncodeField(idText, 63);
            byte[] parsedToken;
            try { parsedToken = EncodeField(tokenText, 127); }
            catch { CryptographicOperations.ZeroMemory(parsedUid); throw; }
            envelope.uid = parsedUid;
            envelope.token = parsedToken;
            envelope.extraInformation = parsedInformation;
            parsedInformation = [];
            envelope.Disposition = "identity-present";
            return envelope;
        }
        catch (Exception ex) when (ex is JsonException or ArgumentException or InvalidDataException)
        {
            return envelope;
        }
        finally { CryptographicOperations.ZeroMemory(owned); CryptographicOperations.ZeroMemory(parsedInformation); }
    }

    // Copies are explicit and remain the caller's responsibility. Nothing in
    // the default probe sends these fields or supplies a successful callback.
    internal (byte[] Uid, byte[] Token) CopyIdentity()
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        if (!IdentityAvailable) throw new InvalidOperationException("No validated identity fields are available.");
        return (uid.ToArray(), token.ToArray());
    }

    internal byte[] CopyExtraInformation()
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        if (Stage != ProbeIdentityStage.Game || !IdentityAvailable)
            throw new InvalidOperationException("Game identity is required for its extra information.");
        return extraInformation.ToArray();
    }

    private static byte[] EncodeField(string? text, int maximum)
    {
        if (string.IsNullOrWhiteSpace(text) || text.Any(char.IsControl))
            throw new InvalidDataException("Invalid identity field.");
        int count = StrictUtf8.GetByteCount(text);
        if (count > maximum) throw new InvalidDataException("Identity exceeds the native field capacity.");
        return StrictUtf8.GetBytes(text);
    }

    private static int Count(JsonElement value, string name) => value.EnumerateObject().Count(property => property.NameEquals(name));

    public void Dispose()
    {
        CryptographicOperations.ZeroMemory(uid);
        CryptographicOperations.ZeroMemory(token);
        CryptographicOperations.ZeroMemory(extraInformation);
        uid = []; token = []; extraInformation = []; disposed = true; Disposition = "disposed";
    }

    public override string ToString() => $"Identity envelope: {Stage}, {Disposition}";
}
