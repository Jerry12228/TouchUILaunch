using System.Globalization;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

// Research-only, ordinary HTTPS authentication. Account operations are called
// by explicit local UI actions or an explicitly authorized saved test session.
// No official account stores or cookies are read.
internal sealed class ProbeAuthenticationClient : IDisposable
{
    private readonly JsonDocument configuration;
    private readonly HttpClient http;
    private readonly bool ownsHttp;
    private string deviceId = Guid.NewGuid().ToString("N");
    internal string DeviceId => deviceId;
    private readonly Action<object> observe;
    private readonly string installedVersion;
    private readonly ProbeGameEnvelopeCodec? gameCodec;
    private readonly string? sessionPath;
    internal bool SessionPersistenceEnabled => sessionPath != null;
    private bool initialized;
    private DateTimeOffset nextSmsUtc;
    internal TimeSpan SmsCooldownRemaining
    {
        get
        {
            TimeSpan remaining = nextSmsUtc - DateTimeOffset.UtcNow;
            return remaining > TimeSpan.Zero ? remaining : TimeSpan.Zero;
        }
    }
    private string? challengedPhone, challengedType;
    internal int Platform { get; }
    internal int ChannelId { get; }
    internal static readonly Uri OneLoginEndpoint = new("https://apidev.laohu.com/sdkapi/v2/user/login");
    internal static readonly Uri ChannelValidationEndpoint = new("https://user.laohu.com/openApi/sdkValidate");
    internal const string ChannelUserAgent = "Mozilla/5.0 (Windows NT 5.1) AppleWebKit/534.30 (KHTML, like Gecko) Chrome/31.0.1650.57 Safari/534.30 AsiaArcClient/1.0";

    internal ProbePreLoginClient CreatePreLoginClient(Nte.Login.NteSdkRegistration registration)
    {
        byte[] key = ProbePreLoginProtocol.KeyFromRegistration(registration);
        try { return new(key, deviceId, installedVersion, observe, deviceProvider: () => deviceId); }
        finally { CryptographicOperations.ZeroMemory(key); }
    }

    internal ProbeAuthenticationClient(JsonElement config, Action<object> observe, HttpClient? http = null, string? installedVersion = null,
        ReadOnlySpan<byte> applicationKeyTail = default, string? sessionPath = null, string? savedDeviceId = null)
    {
        configuration = JsonDocument.Parse(config.GetRawText());
        this.observe = observe;
        this.sessionPath = sessionPath;
        if (savedDeviceId != null)
        {
            if (!Regex.IsMatch(savedDeviceId, @"\A[0-9a-f]{32}\z")) throw new InvalidDataException("Invalid saved device identifier.");
            deviceId = savedDeviceId;
        }
        this.installedVersion = installedVersion ?? configuration.RootElement.GetProperty("meta").GetProperty("version").GetString()!;
        if (!Regex.IsMatch(this.installedVersion, @"\A[0-9]+(?:\.[0-9]+){1,3}\z")) throw new InvalidDataException("Unverified installed version.");
        _ = ProbeAnonymousInit.BuildParameters(configuration.RootElement, DateTimeOffset.UtcNow, deviceId);
        JsonElement meta = configuration.RootElement.GetProperty("meta");
        Platform = meta.GetProperty("platform").GetInt32();
        ChannelId = meta.GetProperty("channelId").GetInt32();
        if (Platform != 4 || ChannelId != 9) throw new InvalidDataException("Unverified PC authentication channel.");
        if (!applicationKeyTail.IsEmpty) gameCodec = new(applicationKeyTail);
        ownsHttp = http == null;
        this.http = http ?? new HttpClient(new HttpClientHandler
        {
            AllowAutoRedirect = false, UseCookies = false, AutomaticDecompression = DecompressionMethods.All
        }) { Timeout = TimeSpan.FromSeconds(20) };
    }

    internal async Task InitializeAsync(CancellationToken token)
    {
        using AuthenticationResponse response = await RequestAsync("anonymous-init", ProbeAnonymousInit.Endpoint,
            Parameters(), false, token).ConfigureAwait(false);
        RequireSuccess(response.RootElement);
        if (!response.RootElement.TryGetProperty("result", out JsonElement result) ||
            !result.TryGetProperty("loginOptionConfig", out JsonElement options) || options.ValueKind != JsonValueKind.Array ||
            !options.EnumerateArray().Any(item => item.ValueKind == JsonValueKind.Number && item.TryGetInt32(out int option) && option == 92))
            throw new ProbeAuthenticationException("server-phone-login-unavailable", null);
        initialized = true;
    }

    internal void PersistSession(ProbeIdentityEnvelope channel)
    {
        if (sessionPath == null) return;
        if (channel.Stage != ProbeIdentityStage.Channel || !channel.IdentityAvailable || gameCodec == null)
            throw new ProbeAuthenticationException("session-save-unavailable", null);
        (byte[] uid, byte[] token) = channel.CopyIdentity(); byte[] key = gameCodec.CopyKey();
        try
        {
            ProbeSessionStore.Save(sessionPath, configuration.RootElement, installedVersion, deviceId, key, uid, token);
            bool verified = ProbeSessionStore.Read(sessionPath, root =>
            {
                byte[] savedUid = root.GetProperty("channelUid").GetBytesFromBase64(), savedToken = root.GetProperty("channelToken").GetBytesFromBase64();
                try { return CryptographicOperations.FixedTimeEquals(savedUid, uid) && CryptographicOperations.FixedTimeEquals(savedToken, token); }
                finally { CryptographicOperations.ZeroMemory(savedUid); CryptographicOperations.ZeroMemory(savedToken); }
            });
            if (!verified) throw new CryptographicException("Protected session verification failed.");
            observe(new { stage = "session-saved", protection = "DPAPI-CurrentUser", readBackVerified = true });
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or CryptographicException or System.ComponentModel.Win32Exception)
        { throw new ProbeAuthenticationException("session-save-failed", null); }
        finally { CryptographicOperations.ZeroMemory(uid); CryptographicOperations.ZeroMemory(token); CryptographicOperations.ZeroMemory(key); }
    }

    internal static (ProbeAuthenticationClient Client, ProbeIdentityEnvelope Channel) RestoreSession(string path, Action<object> observe) => ProbeSessionStore.Read(path, root =>
    {
        byte[] key = root.GetProperty("applicationKeyTail").GetBytesFromBase64();
        byte[] uid = root.GetProperty("channelUid").GetBytesFromBase64(), token = root.GetProperty("channelToken").GetBytesFromBase64();
        byte[] identityJson = [];
        try
        {
            using MemoryStream identity = new();
            using (Utf8JsonWriter writer = new(identity))
            {
                writer.WriteStartObject(); writer.WriteNumber("code", 0); writer.WriteStartObject("result");
                writer.WriteString("userId", uid); writer.WriteString("token", token); writer.WriteEndObject(); writer.WriteEndObject(); writer.Flush();
            }
            identityJson = identity.ToArray();
            if (identity.TryGetBuffer(out ArraySegment<byte> identityBuffer)) CryptographicOperations.ZeroMemory(identityBuffer.AsSpan());
            ProbeIdentityEnvelope channel = ProbeIdentityEnvelope.Parse(ProbeIdentityStage.Channel, 200, identityJson);
            if (!channel.IdentityAvailable) { channel.Dispose(); throw new InvalidDataException("Saved identity fields are invalid."); }
            try
            {
                ProbeAuthenticationClient client = new(root.GetProperty("configuration"), observe,
                    installedVersion: root.GetProperty("installedVersion").GetString(), applicationKeyTail: key,
                    savedDeviceId: root.GetProperty("deviceId").GetString());
                return (client, channel);
            }
            catch { channel.Dispose(); throw; }
        }
        finally { CryptographicOperations.ZeroMemory(key); CryptographicOperations.ZeroMemory(uid); CryptographicOperations.ZeroMemory(token); CryptographicOperations.ZeroMemory(identityJson); }
    });

    internal async Task SendSmsAsync(string phone, CancellationToken token)
    {
        RequireInitialized(); ValidatePhone(phone);
        if (DateTimeOffset.UtcNow < nextSmsUtc) throw new ProbeAuthenticationException("sms-cooldown", null);
        // A failed/ambiguous request also consumes the local cooldown. No retries
        // or automatic SMS sending, even if the HTTP response was lost.
        nextSmsUtc = DateTimeOffset.UtcNow.AddSeconds(60);
        challengedPhone = null; challengedType = null;
        string type = await QuerySmsTypeAsync(phone, token).ConfigureAwait(false);
        SortedDictionary<string, string> values = Parameters();
        values["cellphone"] = phone; values["type"] = type; Sign(values);
        using AuthenticationResponse response = await RequestAsync("send-sms",
            new("https://user.laohu.com/m/newApi/sendPhoneCaptchaWithOutLogin"), values, false, token).ConfigureAwait(false);
        RequireSuccess(response.RootElement);
        challengedPhone = phone; challengedType = type;
    }

    internal async Task<ProbeIdentityEnvelope> LoginChannelAsync(string phone, string captcha, CancellationToken token)
    {
        RequireInitialized(); ValidatePhone(phone);
        if (!Regex.IsMatch(captcha, @"\A[0-9]{4,8}\z")) throw new ProbeAuthenticationException("invalid-captcha", null);
        if (challengedPhone != phone || challengedType == null)
        {
            // A code already received in an earlier test can be entered locally
            // after restarting this host. Query its type, but do not send SMS.
            challengedPhone = null; challengedType = null;
            string type = await QuerySmsTypeAsync(phone, token).ConfigureAwait(false);
            challengedPhone = phone; challengedType = type;
        }
        SortedDictionary<string, string> values = BuildSmsLoginParameters(phone, captcha, challengedType);
        using AuthenticationResponse response = await RequestAsync("channel-login",
            new("https://user.laohu.com/openApi/sms/new/login"), values, false, token).ConfigureAwait(false);
        return ParseIdentity(ProbeIdentityStage.Channel, response.RootElement);
    }

    internal async Task<ProbeIdentityEnvelope> ExchangeGameIdentityAsync(ProbeIdentityEnvelope channel, CancellationToken token)
    {
        RequireInitialized();
        if (channel.Stage != ProbeIdentityStage.Channel || !channel.IdentityAvailable)
            throw new ProbeAuthenticationException("channel-identity-unavailable", null);
        SortedDictionary<string, string> values = BuildExchangeParameters(channel);
        using AuthenticationResponse response = await RequestAsync("game-login", OneLoginEndpoint, values, true, token).ConfigureAwait(false);
        return ParseIdentity(ProbeIdentityStage.Game, response.RootElement);
    }

    internal void RestoreDeviceContext(string savedDeviceId, int platform, int channelId)
    {
        if (platform != Platform || channelId != ChannelId || !Regex.IsMatch(savedDeviceId, @"\A[0-9a-f]{32}\z"))
            throw new InvalidDataException("Saved channel context does not match this client.");
        // Callers cancel the previous attempt and reset admission before this
        // change. All subsequent channel, OneLogin and preLogin requests use it.
        deviceId = savedDeviceId; initialized = false; challengedPhone = challengedType = null;
    }

    internal async Task InitializeRestoredContextAsync(string savedDeviceId, int platform, int channelId, CancellationToken token)
    {
        // A failed saved-account initialization must not strand an already
        // initialized SMS session. Callers serialize attempts and await their
        // cancellation before starting a new one.
        string previousDevice = deviceId;
        bool previousInitialized = initialized;
        token.ThrowIfCancellationRequested();
        RestoreDeviceContext(savedDeviceId, platform, channelId);
        try
        {
            await InitializeAsync(token).ConfigureAwait(false);
            token.ThrowIfCancellationRequested();
        }
        catch
        {
            deviceId = previousDevice; initialized = previousInitialized;
            // Do not carry an SMS challenge across a failed account switch.
            challengedPhone = challengedType = null;
            throw;
        }
    }

    internal async Task<ProbeChannelValidationResult> ValidateChannelIdentityAsync(ProbeIdentityEnvelope channel, CancellationToken token)
    {
        RequireInitialized(); token.ThrowIfCancellationRequested();
        SortedDictionary<string, string> values = BuildChannelValidationParameters(channel);
        try
        {
            using AuthenticationResponse response = await RequestAsync("channel-validation", ChannelValidationEndpoint, values, false, token).ConfigureAwait(false);
            token.ThrowIfCancellationRequested();
            return ProbeChannelValidationResult.Parse(response.RootElement);
        }
        catch (ProbeAuthenticationException ex)
        {
            token.ThrowIfCancellationRequested();
            if (ex.Phase == "http-failure")
            {
                if (ex.ApiCode is int rejected && rejected is not (0 or 400)) return new(ProbeChannelValidationStatus.ServiceRejected, rejected);
                return new(ProbeChannelValidationStatus.RequestRejected, ex.Code);
            }
            return new(ex.Phase == "invalid-response" ? ProbeChannelValidationStatus.UnknownResponse : ProbeChannelValidationStatus.NetworkFailure, null);
        }
        catch (InvalidDataException)
        { token.ThrowIfCancellationRequested(); return new(ProbeChannelValidationStatus.UnknownResponse, null); }
    }

    internal SortedDictionary<string, string> BuildChannelValidationParameters(ProbeIdentityEnvelope channel)
    {
        if (channel.Stage != ProbeIdentityStage.Channel || !channel.IdentityAvailable) throw new InvalidDataException("Channel identity is required.");
        (byte[] uid, byte[] token) = channel.CopyIdentity();
        try
        {
            SortedDictionary<string, string> values = Parameters();
            values["userId"] = Encoding.UTF8.GetString(uid); values["token"] = Encoding.UTF8.GetString(token);
            values["osType"] = "4"; // Verified fixed PC branch, not a platform-independent default.
            Sign(values); return values;
        }
        finally { CryptographicOperations.ZeroMemory(uid); CryptographicOperations.ZeroMemory(token); }
    }

    internal SortedDictionary<string, string> BuildSmsLoginParameters(string phone, string captcha, string type)
    {
        ValidatePhone(phone);
        if (!Regex.IsMatch(captcha, @"\A[0-9]{4,8}\z") || type is not ("16" or "18"))
            throw new InvalidDataException("Invalid SMS authentication input.");
        SortedDictionary<string, string> values = Parameters();
        // These placeholders are part of the SDK's SMS-login wire contract.
        // They are present even on a PC without mobile advertising identifiers.
        values["areaCodeId"] = "1";
        foreach (string field in new[] { "idfa", "adm", "mac" }) values[field] = "";
        values["t"] = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(CultureInfo.InvariantCulture);
        values["cellphone"] = Encrypt(phone); values["captcha"] = Encrypt(captcha); values["type"] = type;
        Sign(values); return values;
    }

    internal SortedDictionary<string, string> BuildExchangeParameters(ProbeIdentityEnvelope channel)
    {
        if (channel.Stage != ProbeIdentityStage.Channel || !channel.IdentityAvailable) throw new InvalidDataException("Channel identity is required.");
        (byte[] uid, byte[] token) = channel.CopyIdentity();
        try
        {
            return ExchangeFields(Encoding.UTF8.GetString(uid), Encoding.UTF8.GetString(token));
        }
        finally { CryptographicOperations.ZeroMemory(uid); CryptographicOperations.ZeroMemory(token); }
    }

    private SortedDictionary<string, string> ExchangeFields(string uid, string token)
    {
        SortedDictionary<string, string> device = ProbeAnonymousInit.BuildParameters(configuration.RootElement, DateTimeOffset.UtcNow, deviceId);
        foreach (string field in new[] { "appId", "channelId", "t", "sign" }) device.Remove(field);
        // NTEGame 0x3F4060 builds a compact JSON string with string-valued
        // channelId/subchannelId. In this verified RCC mode, with no media
        // attribution override, 0x3F4BA6 falls back to the same channel ID.
        string channelId = ChannelId.ToString(CultureInfo.InvariantCulture);
        string extra = JsonSerializer.Serialize(new SortedDictionary<string, string>(StringComparer.Ordinal)
            { ["channelId"] = channelId, ["subchannelId"] = channelId }, NteJsonContext.Default.SortedDictionaryStringString);
        // The fixed producer returns device as a QString containing JSON,
        // then inserts that string into the outer QVariantMap.
        return new(StringComparer.Ordinal)
        {
            ["appId"] = Nte.Login.NteRegionDefinition.CnAppId, ["channelId"] = channelId,
            ["channelUid"] = uid, ["channelToken"] = token, ["extraJson"] = extra,
            ["platformId"] = Platform.ToString(CultureInfo.InvariantCulture), ["version"] = "2", ["device"] = JsonSerializer.Serialize(device, NteJsonContext.Default.SortedDictionaryStringString)
        };
    }

    internal async Task<object> InspectGameEnvelopeWithoutIdentityAsync(CancellationToken token)
    {
        SortedDictionary<string, string> values = ExchangeFields("", "");
        values.Remove("channelUid"); values.Remove("channelToken");
        try
        {
            using AuthenticationResponse response = await RequestAsync("game-envelope-without-identity", OneLoginEndpoint, values, true, token).ConfigureAwait(false);
            JsonElement root = response.RootElement;
            int? code = root.ValueKind == JsonValueKind.Object && root.TryGetProperty("code", out JsonElement field) &&
                field.TryGetInt32(out int number) ? number : null;
            string? message = root.ValueKind == JsonValueKind.Object && root.TryGetProperty("msg", out JsonElement text) && text.ValueKind == JsonValueKind.String
                ? ProbeLoginContract.SafeExplanation(text.GetString()) : null;
            return new { decodedReply = true, serverCode = code, publicValidationMessage = message,
                accountDataSupplied = false, identityAccepted = false, gameCallbackSent = false };
        }
        catch (ProbeAuthenticationException ex)
        {
            return new { decodedReply = false, error = ex.Phase, code = ex.Code,
                accountDataSupplied = false, identityAccepted = false, gameCallbackSent = false };
        }
    }

    internal static string ReadSmsType(JsonElement root)
    {
        if (!root.TryGetProperty("result", out JsonElement result) || result.ValueKind != JsonValueKind.Object ||
            !result.TryGetProperty("havePwd", out JsonElement value)) throw new ProbeAuthenticationException("password-state-unavailable", null);
        bool havePassword = value.ValueKind switch
        {
            JsonValueKind.True => true, JsonValueKind.False => false,
            JsonValueKind.Number when value.TryGetInt32(out int number) && number is 0 or 1 => number == 1,
            _ => throw new ProbeAuthenticationException("invalid-password-state", null)
        };
        return havePassword ? "18" : "16";
    }

    private async Task<string> QuerySmsTypeAsync(string phone, CancellationToken token)
    {
        SortedDictionary<string, string> query = Parameters();
        query["cellphone"] = phone; Sign(query);
        using AuthenticationResponse response = await RequestAsync("phone-password-state",
            new("https://user.laohu.com/m/newApi/query/whetherSetPassword"), query, false, token, queryParameters: true).ConfigureAwait(false);
        RequireSuccess(response.RootElement);
        return ReadSmsType(response.RootElement);
    }

    private SortedDictionary<string, string> Parameters() => BuildChannelParameters(configuration.RootElement, installedVersion, deviceId);

    internal static SortedDictionary<string, string> BuildChannelParameters(JsonElement config, string version, string device)
    {
        // Channel request builders 0x3F9F00/0x3FA100/0x3FA360/0x3FA650
        // differ from the OneLogin device-object builder 0x3E90B0. In particular
        // the package goes into bid, not packageName; deviceType is WindowsPc.
        SortedDictionary<string, string> values = ProbeAnonymousInit.BuildParameters(config, DateTimeOffset.UtcNow, device);
        values.Remove("sign"); values.Remove("appVersion");
        values["bid"] = values["packageName"]; values.Remove("packageName");
        values["deviceName"] = "WindowsPc"; values["deviceType"] = "WindowsPc";
        values["mac"] = ""; values["version"] = version;
        values["t"] = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(CultureInfo.InvariantCulture);
        string key = config.GetProperty("init").GetProperty("channel").GetProperty("channelAppKey").GetString()!;
        byte[] input = Encoding.UTF8.GetBytes(string.Concat(values.Values) + key);
        try { values["sign"] = Convert.ToHexStringLower(MD5.HashData(input)); }
        finally { CryptographicOperations.ZeroMemory(input); }
        return values;
    }
    private string AppKey => configuration.RootElement.GetProperty("init").GetProperty("channel").GetProperty("channelAppKey").GetString()!;
    private void Sign(SortedDictionary<string, string> values)
    {
        values.Remove("sign");
        byte[] bytes = Encoding.UTF8.GetBytes(string.Concat(values.Values) + AppKey);
        try { values["sign"] = Convert.ToHexStringLower(MD5.HashData(bytes)); }
        finally { CryptographicOperations.ZeroMemory(bytes); }
    }

    private string Encrypt(string text)
    {
        byte[] key = Encoding.ASCII.GetBytes(AppKey[16..]), input = Encoding.UTF8.GetBytes(text);
        byte[] ciphertext = [];
        try
        {
            using Aes aes = Aes.Create(); aes.Key = key;
            ciphertext = aes.EncryptEcb(input, PaddingMode.PKCS7);
            return Convert.ToBase64String(ciphertext);
        }
        finally { CryptographicOperations.ZeroMemory(key); CryptographicOperations.ZeroMemory(input); CryptographicOperations.ZeroMemory(ciphertext); }
    }

    private async Task<AuthenticationResponse> RequestAsync(string stage, Uri endpoint, SortedDictionary<string, string> values,
        bool oneLogin, CancellationToken token, bool queryParameters = false)
    {
        int? status = null, code = null;
        string? missingRequiredField = null;
        string? localExplanation = null;
        bool responseParsed = false;
        string[] failureIndicators = [];
        byte[] body = [], buffer = new byte[8192], wireBody = [];
        try
        {
            // SmsLoginTask RVA 0xBF103C sets POST=false only for mode 3
            // (whetherSetPassword). The other SMS routes use form POST.
            if (oneLogin)
            {
                if (gameCodec == null) throw new ProbeAuthenticationException("game-key-unavailable", null);
                byte[] json = JsonSerializer.SerializeToUtf8Bytes(values, NteJsonContext.Relaxed.SortedDictionaryStringString);
                try { wireBody = gameCodec.Encode(json); }
                finally { CryptographicOperations.ZeroMemory(json); }
            }
            using HttpContent content = oneLogin ? new ByteArrayContent(wireBody) : new FormUrlEncodedContent(values);
            if (oneLogin) content.Headers.ContentType = new("application/x-www-form-urlencoded");
            Uri destination = queryParameters ? new UriBuilder(endpoint)
                { Query = await content.ReadAsStringAsync(token).ConfigureAwait(false) }.Uri : endpoint;
            using HttpRequestMessage request = new(queryParameters ? HttpMethod.Get : HttpMethod.Post, destination)
                { Content = queryParameters ? null : content };
            request.Headers.Accept.ParseAdd("application/json");
            // The fixed client's CURL transport sets this public header at
            // RVA 0xC187C2 (CURLOPT_USERAGENT). Validate the same wire context.
            if (endpoint == ChannelValidationEndpoint) request.Headers.UserAgent.ParseAdd(ChannelUserAgent);
            if (oneLogin) request.Headers.Add("appId", Nte.Login.NteRegionDefinition.CnAppId);
            using HttpResponseMessage response = await http.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, token).ConfigureAwait(false);
            status = (int)response.StatusCode;
            if (response.Content.Headers.ContentLength > 1024 * 1024) throw new InvalidDataException("Authentication response size bound.");
            await using Stream stream = await response.Content.ReadAsStreamAsync(token).ConfigureAwait(false);
            using MemoryStream output = new();
            try
            {
                int count;
                while ((count = await stream.ReadAsync(buffer, token).ConfigureAwait(false)) != 0)
                {
                    if (output.Length + count > 1024 * 1024) throw new InvalidDataException("Authentication response size bound.");
                    output.Write(buffer, 0, count);
                }
                body = output.ToArray();
            }
            finally { if (output.TryGetBuffer(out ArraySegment<byte> segment)) CryptographicOperations.ZeroMemory(segment.AsSpan()); }
            if (!response.IsSuccessStatusCode)
            {
                string diagnosticText = Encoding.UTF8.GetString(body);
                failureIndicators = new[] { "required", "invalid", "signature", "sign", "missing", "token", "userId", "uid", "osType", "timestamp", "parameter", "appId", "format", "json", "decrypt", "签名", "参数", "解密", "过期" }
                    .Where(word => diagnosticText.Contains(word, StringComparison.OrdinalIgnoreCase)).ToArray();
                // Keep only a numeric API code and a known required field name.
                // Error text can echo an account or credential: never log it.
                try
                {
                    using JsonDocument failure = JsonDocument.Parse(body);
                    responseParsed = true;
                    JsonElement root = failure.RootElement;
                    if (root.ValueKind == JsonValueKind.Object)
                    {
                        if (root.TryGetProperty("code", out JsonElement apiCode) && apiCode.ValueKind == JsonValueKind.Number && apiCode.TryGetInt32(out int apiNumber)) code = apiNumber;
                        foreach (string field in new[] { "message", "msg", "error" })
                        {
                            if (!root.TryGetProperty(field, out JsonElement message) || message.ValueKind != JsonValueKind.String) continue;
                            string text = message.GetString() ?? "";
                            if (text.Length > 4096) continue;
                            localExplanation ??= ProbeLoginContract.SafeExplanation(text);
                            Match match = Regex.Match(text, "Required request (?:parameter|header) ['\"]([A-Za-z][A-Za-z0-9]{0,31})['\"]");
                            if (match.Success) missingRequiredField = match.Groups[1].Value;
                        }
                    }
                }
                catch (JsonException) { }
                throw new ProbeAuthenticationException("http-failure", status, localExplanation, code);
            }
            if (oneLogin)
            {
                byte[] decoded = gameCodec!.Decode(body);
                CryptographicOperations.ZeroMemory(body); body = decoded;
            }
            JsonDocument document = JsonDocument.Parse(body, new JsonDocumentOptions { MaxDepth = 32 });
            if (document.RootElement.ValueKind == JsonValueKind.Object && document.RootElement.TryGetProperty("code", out JsonElement value) &&
                value.ValueKind == JsonValueKind.Number && value.TryGetInt32(out int number)) code = number;
            observe(new { stage, httpStatus = status, serverCode = code, responseParsed = true });
            AuthenticationResponse result = new(document, body);
            body = []; // Transfer ownership; JsonDocument still reads this memory.
            return result;
        }
        catch (ProbeAuthenticationException ex)
        {
            observe(new { stage, httpStatus = status, serverCode = code, error = ex.Phase, responseParsed, missingRequiredField, failureIndicators }); throw;
        }
        catch (Exception ex) when (ex is HttpRequestException or IOException or JsonException or OperationCanceledException or InvalidOperationException or CryptographicException or FormatException)
        {
            observe(new { stage, httpStatus = status, serverCode = code, error = ex.GetType().Name });
            throw new ProbeAuthenticationException(token.IsCancellationRequested ? "cancelled" : ex is JsonException or FormatException ? "invalid-response" : "request-failed", code);
        }
        finally { values.Clear(); CryptographicOperations.ZeroMemory(body); CryptographicOperations.ZeroMemory(buffer); CryptographicOperations.ZeroMemory(wireBody); }
    }

    private static ProbeIdentityEnvelope ParseIdentity(ProbeIdentityStage stage, JsonElement response)
    {
        byte[] bytes = JsonSerializer.SerializeToUtf8Bytes(response, NteJsonContext.Default.JsonElement);
        try
        {
            ProbeIdentityEnvelope identity = ProbeIdentityEnvelope.Parse(stage, 200, bytes);
            if (!identity.IdentityAvailable)
            {
                int? code = identity.ServerCode; identity.Dispose();
                throw new ProbeAuthenticationException(stage == ProbeIdentityStage.Channel ? "channel-login-rejected" : "game-login-rejected", code);
            }
            return identity;
        }
        finally { CryptographicOperations.ZeroMemory(bytes); }
    }

    private void RequireInitialized() { if (!initialized) throw new ProbeAuthenticationException("not-initialized", null); }
    private static void ValidatePhone(string phone)
    {
        if (!Regex.IsMatch(phone, @"\A1[0-9]{10}\z")) throw new ProbeAuthenticationException("invalid-mainland-phone", null);
    }
    private static void RequireSuccess(JsonElement root)
    {
        if (root.ValueKind != JsonValueKind.Object || !root.TryGetProperty("code", out JsonElement value) ||
            value.ValueKind != JsonValueKind.Number || !value.TryGetInt32(out int code)) throw new ProbeAuthenticationException("invalid-response", null);
        if (code != 0) throw new ProbeAuthenticationException("server-rejected", code);
    }

    public void Dispose() { initialized = false; challengedPhone = null; challengedType = null; gameCodec?.Dispose(); configuration.Dispose(); if (ownsHttp) http.Dispose(); }

    private sealed class AuthenticationResponse(JsonDocument document, byte[] bytes) : IDisposable
    {
        internal JsonElement RootElement => document.RootElement;
        public void Dispose() { document.Dispose(); CryptographicOperations.ZeroMemory(bytes); }
    }
}

internal sealed class ProbeAuthenticationException(string phase, int? code, string? localExplanation = null, int? apiCode = null) : Exception(phase)
{
    internal string Phase { get; } = phase;
    internal int? Code { get; } = code;
    internal int? ApiCode { get; } = apiCode;
    // Displayed locally only; never serialize this exception or its properties.
    internal string? LocalExplanation { get; } = localExplanation;
}
