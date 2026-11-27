using System.Buffers.Binary;
using System.Globalization;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using Nte.Login;

// Fixed-image protocol, isolated from the production plugin. The service's
// complete result is forwarded unchanged; no local "allowed" result exists.
internal readonly record struct ProbePreLoginRequest(int ContextId)
{
    internal static int ReadQueryType(ReadOnlySpan<byte> frame, int clientId)
    {
        if (clientId <= 0 || frame.Length != 0x214 || BinaryPrimitives.ReadUInt32LittleEndian(frame) != 0x6B647331 ||
            BinaryPrimitives.ReadUInt32LittleEndian(frame[4..]) != 40 || BinaryPrimitives.ReadInt32LittleEndian(frame[8..]) != frame.Length)
            throw new InvalidDataException("Unverified pre-login request frame.");
        Span<byte> type = stackalloc byte[4];
        frame.Slice(12, 4).CopyTo(type);
        for (int i = 0; i < type.Length; i++) type[i] ^= unchecked((byte)clientId);
        return BinaryPrimitives.ReadInt32LittleEndian(type);
    }

    internal static ProbePreLoginRequest Parse(ReadOnlySpan<byte> frame, int clientId, int queryType = 1)
    {
        if (queryType is not (1 or 2) || ReadQueryType(frame, clientId) != queryType)
            throw new InvalidDataException("Unverified antifatigue query type.");
        byte[] body = frame[12..].ToArray();
        try
        {
            for (int i = 0; i < body.Length; i++) body[i] ^= unchecked((byte)clientId);
            // Pre-login and the native info subscription both pass "{}".
            if (BinaryPrimitives.ReadInt32LittleEndian(body) != queryType || !body.AsSpan(4, 3).SequenceEqual("{}\0"u8) ||
                body.AsSpan(7, 509).IndexOfAnyExcept((byte)0) >= 0)
                throw new InvalidDataException("Unverified pre-login query type or payload.");
            int context = BinaryPrimitives.ReadInt32LittleEndian(body.AsSpan(0x204));
            if (queryType == 2 && context != -1) throw new InvalidDataException("Unverified antifatigue subscription context.");
            return new(context);
        }
        finally { CryptographicOperations.ZeroMemory(body); }
    }
}

internal static class ProbePreLoginProtocol
{
    // Native GameAntifatigue constructor 0xC094C0, using the full static APPKEY.
    // This is different from both the channel key and OneLogin's final 16 bytes.
    internal static byte[] DeriveKey(ReadOnlySpan<byte> applicationKey)
    {
        if (applicationKey.Length != 32 || applicationKey.IndexOfAnyInRange((byte)0, (byte)31) >= 0 ||
            applicationKey.IndexOfAnyInRange((byte)127, byte.MaxValue) >= 0)
            throw new InvalidDataException("Unverified application key shape.");
        byte[] key = new byte[16];
        applicationKey.Slice(1, 2).CopyTo(key);
        applicationKey.Slice(7, 5).CopyTo(key.AsSpan(2));
        applicationKey.Slice(15, 3).CopyTo(key.AsSpan(7));
        applicationKey.Slice(25, 6).CopyTo(key.AsSpan(10));
        return key;
    }

    internal static byte[] KeyFromRegistration(NteSdkRegistration registration)
    {
        byte[] appKey = registration.ReadCnApplicationKey();
        try { return DeriveKey(appKey); }
        finally { CryptographicOperations.ZeroMemory(appKey); }
    }

    internal static string Sign(SortedDictionary<string, string> fields, ReadOnlySpan<byte> key)
    {
        if (key.Length != 16 || fields.ContainsKey("sign")) throw new InvalidDataException("Unverified pre-login signing input.");
        // 0xC1DE90: key + ordinal-sorted raw key=value pairs, separated by &;
        // 0xC221F0/0xB9B280: SHA-256 then standard padded Base64, before form encoding.
        byte[] pairs = Encoding.UTF8.GetBytes(string.Join('&', fields.Select(pair => pair.Key + "=" + pair.Value)));
        byte[] input = new byte[key.Length + pairs.Length];
        key.CopyTo(input); pairs.CopyTo(input, key.Length);
        byte[] digest = [];
        try { digest = SHA256.HashData(input); return Convert.ToBase64String(digest); }
        finally { CryptographicOperations.ZeroMemory(pairs); CryptographicOperations.ZeroMemory(input); CryptographicOperations.ZeroMemory(digest); }
    }
}

internal sealed class ProbePreLoginResponse : IDisposable
{
    private byte[] bytes;
    private readonly string message;
    private bool disposed;
    internal int Code { get; }
    internal int? Status { get; }
    internal int? RealUser { get; }
    internal int? BannedType { get; }
    internal ProbeLoginDecision Decision => ProbeLoginDecision.Evaluate(Code, Status, RealUser, BannedType);
    internal object Summary => new { serverCode = Code, status = Status, realUser = RealUser, bannedType = BannedType,
        responseBytes = bytes.Length, responseParsed = true };

    private ProbePreLoginResponse(byte[] bytes, int code, string message, int? status, int? realUser, int? bannedType)
    { this.bytes = bytes; Code = code; this.message = message; Status = status; RealUser = realUser; BannedType = bannedType; }

    internal static ProbePreLoginResponse Parse(ReadOnlySpan<byte> response, ReadOnlySpan<byte> expectedUserId = default)
    {
        if (response.Length is 0 or > 65536 - 0x422 || response.Contains((byte)0))
            throw new InvalidDataException("Pre-login response exceeds the verified native callback capacity.");
        byte[] owned = response.ToArray();
        try
        {
            using JsonDocument document = JsonDocument.Parse(owned, new JsonDocumentOptions { MaxDepth = 32 });
            JsonElement root = document.RootElement;
            if (root.ValueKind != JsonValueKind.Object || Count(root, "code") != 1 ||
                root.GetProperty("code").ValueKind != JsonValueKind.Number || !root.GetProperty("code").TryGetInt32(out int code) ||
                Count(root, "message") > 1)
                throw new InvalidDataException("Unverified pre-login response contract.");
            string message = root.TryGetProperty("message", out JsonElement text) && text.ValueKind == JsonValueKind.String ? text.GetString()! : "";
            if (message.Contains('\0') || message.Length > 511) throw new InvalidDataException("Pre-login message exceeds the native field capacity.");
            int? status = null, realUser = null, bannedType = null;
            if (code == 0)
            {
                if (Count(root, "result") != 1 || root.GetProperty("result").ValueKind != JsonValueKind.Object)
                    throw new InvalidDataException("Missing pre-login result.");
                JsonElement result = root.GetProperty("result");
                status = Number(result, "status"); realUser = Number(result, "realUser"); bannedType = Number(result, "bannedType");
                // The live native service explicitly returns null for an absent
                // ban. Preserve that null in the untouched body, not a local 0.
                if (status == null || realUser == null || Count(result, "bannedType") != 1 ||
                    (bannedType == null && result.GetProperty("bannedType").ValueKind != JsonValueKind.Null))
                    throw new InvalidDataException("Incomplete pre-login restrictions.");
                if (!expectedUserId.IsEmpty)
                {
                    if (Number(result, "appId") != NteRegionDefinition.CN.ApplicationId || Count(result, "userId") != 1 || result.GetProperty("userId").ValueKind != JsonValueKind.String)
                        throw new InvalidDataException("Unverified pre-login identity binding.");
                    byte[] returnedId = Encoding.UTF8.GetBytes(result.GetProperty("userId").GetString()!);
                    try
                    {
                        if (!CryptographicOperations.FixedTimeEquals(returnedId, expectedUserId))
                            throw new InvalidDataException("Pre-login result belongs to a different identity.");
                    }
                    finally { CryptographicOperations.ZeroMemory(returnedId); }
                }
            }
            return new(response.ToArray(), code, message, status, realUser, bannedType);
        }
        finally { CryptographicOperations.ZeroMemory(owned); }
    }

    internal static object DescribeContract(ReadOnlyMemory<byte> response)
    {
        using JsonDocument document = JsonDocument.Parse(response, new JsonDocumentOptions { MaxDepth = 32 });
        JsonElement root = document.RootElement;
        JsonElement result = root.ValueKind == JsonValueKind.Object && root.TryGetProperty("result", out JsonElement value) ? value : default;
        return new
        {
            stage = "antifatigue-response-contract",
            serverCode = root.ValueKind == JsonValueKind.Object ? Number(root, "code") : null,
            status = result.ValueKind == JsonValueKind.Object ? Number(result, "status") : null,
            realUser = result.ValueKind == JsonValueKind.Object ? Number(result, "realUser") : null,
            bannedType = result.ValueKind == JsonValueKind.Object ? Number(result, "bannedType") : null,
            rootFields = Names(root), resultFields = Names(result)
        };

        static object[] Names(JsonElement parent) => parent.ValueKind != JsonValueKind.Object ? [] : parent.EnumerateObject()
            .Where(property => Regex.IsMatch(property.Name, @"\A[A-Za-z][A-Za-z0-9_]{0,39}\z"))
            .Take(32).Select(property => (object)new { name = property.Name, type = property.Value.ValueKind.ToString() }).ToArray();
    }

    internal byte[] Encode(ProbePreLoginRequest request, int hostId)
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        if (hostId <= 0) throw new ArgumentOutOfRangeException(nameof(hostId));
        // Official generic query sender 0x4736E0: operation 49 wraps query
        // type 1. The bridge 0x33E39 uses contextId to consume the exact callback.
        byte[] frame = new byte[checked(bytes.Length + 0x422)];
        BinaryPrimitives.WriteUInt32LittleEndian(frame, 0x6B647331);
        BinaryPrimitives.WriteInt32LittleEndian(frame.AsSpan(4), 49);
        BinaryPrimitives.WriteInt32LittleEndian(frame.AsSpan(8), frame.Length);
        BinaryPrimitives.WriteInt32LittleEndian(frame.AsSpan(12), bytes.Length + 0x411);
        BinaryPrimitives.WriteInt32LittleEndian(frame.AsSpan(16), 1);
        BinaryPrimitives.WriteInt32LittleEndian(frame.AsSpan(20), Code);
        BinaryPrimitives.WriteInt32LittleEndian(frame.AsSpan(24), request.ContextId);
        Encoding.Unicode.GetBytes(message, frame.AsSpan(28, 1024));
        bytes.CopyTo(frame, 0x41C);
        for (int i = 12; i < frame.Length; i++) frame[i] ^= unchecked((byte)hostId);
        return frame;
    }

    private static int Count(JsonElement value, string key) => value.EnumerateObject().Count(property => property.NameEquals(key));
    private static int? Number(JsonElement value, string key)
    {
        if (Count(value, key) != 1) return null;
        JsonElement field = value.GetProperty(key);
        return field.ValueKind == JsonValueKind.Number && field.TryGetInt32(out int number) ? number :
            field.ValueKind == JsonValueKind.String && int.TryParse(field.GetString(), NumberStyles.AllowLeadingSign, CultureInfo.InvariantCulture, out number) ? number : null;
    }
    public void Dispose() { CryptographicOperations.ZeroMemory(bytes); bytes = []; disposed = true; }
    public override string ToString() => "Pre-login response (payload withheld)";
}

internal sealed class ProbePreLoginClient : IDisposable
{
    internal static readonly Uri Endpoint = new("https://antifatigue.laohu.com/sdkapi/preLogin");
    private readonly byte[] key;
    private readonly string deviceId, installedVersion;
    private readonly Func<string>? deviceProvider;
    private readonly HttpClient http;
    private readonly bool ownsHttp;
    private readonly Action<object> observe;
    private byte[] uid = [];
    private bool disposed;

    internal ProbePreLoginClient(ReadOnlySpan<byte> key, string deviceId, string installedVersion, Action<object> observe, HttpClient? http = null, Func<string>? deviceProvider = null)
    {
        if (key.Length != 16 || !Regex.IsMatch(deviceId, @"\A[0-9a-f]{32}\z") ||
            !Regex.IsMatch(installedVersion, @"\A[0-9]+(?:\.[0-9]+){1,3}\z")) throw new InvalidDataException("Unverified pre-login context.");
        this.key = key.ToArray(); this.deviceId = deviceId; this.installedVersion = installedVersion; this.observe = observe;
        this.deviceProvider = deviceProvider;
        ownsHttp = http == null;
        this.http = http ?? new HttpClient(new HttpClientHandler
        { AllowAutoRedirect = false, UseCookies = false, AutomaticDecompression = DecompressionMethods.All }) { Timeout = TimeSpan.FromSeconds(20) };
    }

    internal void ClearIdentity()
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        CryptographicOperations.ZeroMemory(uid); uid = [];
    }

    internal void SetIdentity(ProbeIdentityEnvelope identity)
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        if (identity.Stage != ProbeIdentityStage.Game || !identity.IdentityAvailable) throw new InvalidDataException("Game identity required for pre-login.");
        (byte[] id, byte[] token) = identity.CopyIdentity();
        CryptographicOperations.ZeroMemory(token); CryptographicOperations.ZeroMemory(uid); uid = id;
    }

    internal SortedDictionary<string, string> BuildParameters(DateTimeOffset time)
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        if (uid.Length == 0) throw new InvalidOperationException("Pre-login requires a game identity.");
        string currentDevice = deviceProvider?.Invoke() ?? deviceId;
        if (!Regex.IsMatch(currentDevice, @"\A[0-9a-f]{32}\z")) throw new InvalidDataException("Invalid active device context.");
        // 0xC0A700 common builder and 0x584A50 version setter. Both device
        // fields use this host's own persisted identifier, never vendor data.
        SortedDictionary<string, string> fields = new(StringComparer.Ordinal)
        {
            ["appId"] = NteRegionDefinition.CnAppId, ["channelId"] = "9", ["platformId"] = "4", ["userId"] = Encoding.UTF8.GetString(uid),
            ["udid"] = currentDevice, ["ndid"] = currentDevice, ["deviceName"] = "WindowsPc",
            ["version"] = "2", ["appVersion"] = installedVersion, ["sdkVersion"] = NteCnSdkCompatibility.ServiceSdkVersion,
            ["timestamp"] = time.ToUnixTimeMilliseconds().ToString(CultureInfo.InvariantCulture)
        };
        fields["sign"] = ProbePreLoginProtocol.Sign(fields, key); return fields;
    }

    internal async Task<ProbePreLoginResponse> QueryAsync(CancellationToken token)
    {
        int? status = null;
        byte[] body = [], buffer = new byte[8192];
        SortedDictionary<string, string> fields = BuildParameters(DateTimeOffset.UtcNow);
        try
        {
            using HttpRequestMessage request = new(HttpMethod.Post, Endpoint) { Content = new FormUrlEncodedContent(fields) };
            request.Headers.Accept.ParseAdd("application/json");
            using HttpResponseMessage response = await http.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, token).ConfigureAwait(false);
            status = (int)response.StatusCode;
            if (!response.IsSuccessStatusCode) throw new HttpRequestException("Pre-login transport failure.");
            const int maximum = 65536 - 0x422;
            if (response.Content.Headers.ContentLength > maximum) throw new InvalidDataException("Pre-login response size bound.");
            await using Stream stream = await response.Content.ReadAsStreamAsync(token).ConfigureAwait(false);
            using MemoryStream output = new();
            try
            {
                int count;
                while ((count = await stream.ReadAsync(buffer, token).ConfigureAwait(false)) != 0)
                {
                    if (output.Length + count > maximum) throw new InvalidDataException("Pre-login response size bound.");
                    output.Write(buffer, 0, count);
                }
                body = output.ToArray();
            }
            finally { if (output.TryGetBuffer(out ArraySegment<byte> memory)) CryptographicOperations.ZeroMemory(memory.AsSpan()); }
            observe(ProbePreLoginResponse.DescribeContract(body));
            ProbePreLoginResponse result = ProbePreLoginResponse.Parse(body, uid);
            observe(new { stage = "antifatigue-prelogin", httpStatus = status, serverCode = result.Code, responseParsed = true,
                status = result.Status, realUser = result.RealUser, bannedType = result.BannedType });
            return result;
        }
        catch (Exception ex) when (ex is HttpRequestException or IOException or InvalidDataException or JsonException or InvalidOperationException or OperationCanceledException)
        {
            observe(new { stage = "antifatigue-prelogin", httpStatus = status, error = ex.GetType().Name }); throw;
        }
        finally { fields.Clear(); CryptographicOperations.ZeroMemory(body); CryptographicOperations.ZeroMemory(buffer); }
    }
    public void Dispose() { disposed = true; CryptographicOperations.ZeroMemory(key); CryptographicOperations.ZeroMemory(uid); uid = []; if (ownsHttp) http.Dispose(); }
}
