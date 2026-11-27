using System.Globalization;
using System.Net;
using System.Net.Mail;
using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Nte.Login;

internal sealed partial class NteGlobalAuthentication : IDisposable
{
    private readonly NteOverseasApplicationConfiguration configuration;
    private readonly HttpClient http;
    private readonly bool ownsHttp;
    private JsonDocument? initialization;
    internal NteOverseasRegion Region => configuration.Region;
    internal string DeviceId { get; }
    private readonly string language;
    private DateTimeOffset nextCaptcha;
    internal bool Initialized => initialization != null;
    internal bool SupportsEmail => Region.HostRegion == NteHostRegion.Global;

    internal TimeSpan CaptchaCooldown =>
        nextCaptcha > DateTimeOffset.UtcNow ? nextCaptcha - DateTimeOffset.UtcNow : TimeSpan.Zero;

    internal string ClientData { get; }

    internal NteGlobalAuthentication(NteOverseasApplicationConfiguration configuration, string version,
        HttpClient? http = null, string language = "en", string? deviceId = null)
    {
        DeviceId = deviceId ?? Guid.NewGuid().ToString("N");
        if (DeviceId.Length != 32 || DeviceId.Any(c => !char.IsAsciiHexDigit(c)))
            throw new ArgumentException("Invalid device context.", nameof(deviceId));
        this.configuration = configuration;
        this.language = configuration.Region.HostRegion == NteHostRegion.Taiwan ? "zh-tw" : language;
        ownsHttp = http == null;
        this.http = http ?? new(new HttpClientHandler { AllowAutoRedirect = false, UseCookies = false })
            { Timeout = TimeSpan.FromSeconds(30) };
        ClientData = Uri.EscapeDataString(JsonSerializer.Serialize(new Dictionary<string, object?>
        {
            ["appId"] = Region.ApplicationId,
            ["packageName"] = configuration.PackageName,
            ["appVersion"] = version,
            ["sdkVersion"] = Region.Sdk.AuthenticationVersion,
            ["sdkServerVersion"] = Region.Sdk.AuthenticationServerVersion,
            ["osType"] = 4,
            ["udid"] = DeviceId,
            ["ndid"] = DeviceId,
            ["deviceName"] = "PC",
            ["phoneSystemVersion"] = Environment.OSVersion.VersionString,
            ["channelId"] = 0,
            ["mediaId"] = "",
            ["afId"] = "",
            ["deviceLanguage"] = CultureInfo.CurrentUICulture.Name,
            ["appLanguage"] = this.language,
            ["deviceTimeZone"] = TimeZoneInfo.Local.Id,
            ["androidAdId"] = "",
            ["pcAdid"] = "",
            ["ctz"] = TimeZoneInfo.Local.BaseUtcOffset.ToString(),
            ["ctzid"] = TimeZoneInfo.Local.Id,
            ["csl"] = CultureInfo.CurrentUICulture.Name,
            ["gameClientLang"] = this.language,
            ["gameServerTz"] = "",
            ["serverId"] = ""

        }, NteJsonContext.Default.DictionaryStringObject));
    }

    internal async Task InitializeAsync(CancellationToken token)
    {
        using JsonDocument response = await RequestAsync("/c/api/config/getV2", [], token);
        JsonDocument next = JsonDocument.Parse(RequireResult(response.RootElement).GetRawText());
        initialization?.Dispose();
        initialization = next;
    }

    internal async Task SendEmailCaptchaAsync(string email, CancellationToken token)
    {
        if (!SupportsEmail) throw new NteGlobalAuthenticationException("LoginMethodUnavailable");
        RequireReady();
        ValidateEmail(email);
        if (CaptchaCooldown > TimeSpan.Zero) throw new NteGlobalAuthenticationException("EmailCooldown");
        // Ambiguous delivery also consumes the cooldown. No automatic retries.
        nextCaptcha = DateTimeOffset.UtcNow.AddSeconds(60);
        using JsonDocument reply = await RequestAsync("/c/api/email/captcha/get",
            new() { ["email"] = configuration.EncodeEmail(email) }, token);
        RequireSuccess(reply.RootElement);
    }

    internal async Task<NteGlobalIdentity> LoginEmailAsync(string email, string secret, bool password,
        CancellationToken token)
    {
        if (!SupportsEmail) throw new NteGlobalAuthenticationException("LoginMethodUnavailable");
        RequireReady();
        ValidateEmail(email);
        if (secret.Length is < 1 or > 256 || secret.Contains('\0'))
            throw new NteGlobalAuthenticationException("InvalidSecret");
        Dictionary<string, string> fields = new() { ["email"] = configuration.EncodeEmail(email) };
        if (password)
        {
            fields["password"] = EncryptPassword(secret);
        }
        else fields["captcha"] = secret;

        using JsonDocument response =
            await RequestAsync(password ? "/c/api/email/login/password" : "/c/api/email/captcha/login", fields, token);
        return NteGlobalIdentity.FromService(response.RootElement, 8, Region).WithProfile(NteGlobalProfile.FromEmail(email));
    }

    internal Uri CreateBrowserLoginUri(int provider, string callbackTarget)
    {
        RequireReady();
        if (provider is not (1 or 2) ||
            !System.Text.RegularExpressions.Regex.IsMatch(callbackTarget, @"\Acollapse-nte-[a-f0-9]{32}\z"))
            throw new ArgumentException("Invalid global browser login context.");
        JsonElement accounts = initialization!.RootElement.GetProperty("thirdAccount");

        string Decode(JsonElement parent, string key) =>
            configuration.DecodeServiceValue(parent.GetProperty(key).GetString()!);

        string name = provider == 1 ? "facebook" : "google";
        Uri page = new(Decode(accounts.GetProperty(name), "loginUrl"));
        if (!Region.IsBrowserOrigin(page) ||
            page.AbsolutePath != "/pcLogin_3.0/" + name + ".html")
            throw new InvalidDataException("Unexpected global browser origin.");
        Dictionary<string, object> settings = new()
        {
            ["appId"] = Region.AppId, ["packageName"] = configuration.PackageName, ["webdomain"] = Region.WebService.GetLeftPart(UriPartial.Authority),
            ["directURL"] = callbackTarget, ["funType"] = 1, ["xClientData"] = ClientData, ["hl"] = language
        };
        if (provider == 1)
        {
            settings["facebookAppId"] = Decode(accounts.GetProperty("facebook"), "appId");
            settings["fbsdkjsurl"] = Decode(accounts.GetProperty("facebook"), "sdkJsUrl");
        }
        else
        {
            JsonElement firebase = accounts.GetProperty("firebase");
            settings["firebaseConfig"] = new Dictionary<string, string>
            {
                ["apiKey"] = Decode(firebase, "apiKey"), ["authDomain"] = Decode(firebase, "authDomain"),
                ["projectId"] = Decode(firebase, "projectId"),
                ["storageBucket"] = Decode(firebase, "storageBucket"),
                ["messagingSenderId"] = Decode(firebase, "messagingSenderId"),
                ["appId"] = Decode(firebase, "appId"), ["measurementId"] = Decode(firebase, "measurementId"),
                ["databaseURL"] = "https://pwrdsdk.firebaseio.com"
            };
            settings["firebasejsURL"] = Decode(firebase, "jsUrl");
        }

        string data = JsonSerializer.Serialize(new Dictionary<string, object?>
        {
            ["code"] = 0,
            ["result"] = settings
        }, NteJsonContext.Default.DictionaryStringObject);
        return new(page.GetLeftPart(UriPartial.Path) + "?webLoginType=out&lang=" + Uri.EscapeDataString(language) +
                   "&callback=" + Uri.EscapeDataString(data));
    }

    internal async Task<NteGlobalIdentity> CompleteBrowserLoginAsync(JsonElement result, int provider,
        CancellationToken token)
    {
        RequireReady();
        if (provider is not (1 or 2) || result.ValueKind != JsonValueKind.Object ||
            !result.TryGetProperty("thirdReqParams", out JsonElement third) ||
            third.ValueKind != JsonValueKind.Object ||
            !third.TryGetProperty("thirdType", out JsonElement type) || type.ValueKind != JsonValueKind.Number ||
            !type.TryGetInt32(out int actual) || actual != provider)
            throw new InvalidDataException("Unexpected browser provider result.");
        // Match GlobalLoginController::_doThirdLoginFromCallbackUrl: the web
        // result supplies provider credentials; the PC service creates this
        // client's identity. Its top-level web uid/token are not PC credentials.
        Dictionary<string, string> fields = new() { ["thirdType"] = provider.ToString(CultureInfo.InvariantCulture) };
        foreach (string key in new[]
                 {
                     "thirdAuthToken", "thirdAvatar", "thirdId", "thirdUsername", "thirdEmail", "googleId",
                     "appleIdentityToken", "thirdCode"
                 })
        {
            string value = third.TryGetProperty(key, out JsonElement field) && field.ValueKind == JsonValueKind.String
                ? field.GetString()!
                : "";
            if (value.Length > 16 * 1024 || value.Contains('\0'))
                throw new InvalidDataException("Invalid browser provider field.");
            fields[key] = value;
        }

        if (fields["thirdAuthToken"].Length == 0)
            throw new InvalidDataException("Missing browser provider credential.");
        using JsonDocument response = await RequestAsync("/c/api/user/third/login", fields, token);
        NteGlobalIdentity identity = NteGlobalIdentity.FromService(response.RootElement, provider, Region);
        string? googleProject = null;
        if (provider == 2)
        {
            try
            {
                googleProject = configuration.DecodeServiceValue(initialization!.RootElement.GetProperty("thirdAccount")
                    .GetProperty("firebase").GetProperty("messagingSenderId").GetString()!);
            }
            catch (Exception error) when (error is KeyNotFoundException or InvalidOperationException or FormatException
                                              or CryptographicException or InvalidDataException)
            {
                /* Optional display data must not replace official authentication. */
            }
        }

        return identity.WithProfile(NteGlobalProfile.AfterBrowserExchange(provider, fields, googleProject));
    }

    internal Task<NteGlobalIdentity> RestoreSessionAsync(NteGlobalIdentity identity, CancellationToken token)
    {
        if (identity.Region != Region) throw new InvalidDataException("The saved account belongs to another publisher.");
        RequireReady();
        return ValidateIdentityAsync(NteGlobalIdentity.ReadUserId(identity.Result.GetProperty("uid")),
            identity.Result.GetProperty("token").GetString()!, identity.Provider, token);
    }

    internal Task<JsonDocument> QuerySdkInfoAsync(NteGlobalSdkQuery query, CancellationToken token)
        => RequestAsync(query.ServicePath, [], token);

    private async Task<NteGlobalIdentity> ValidateIdentityAsync(string uid, string accessToken, int provider,
        CancellationToken token)
    {
        using JsonDocument response = await RequestAsync("/c/api/user/token/login",
            new() { ["uid"] = uid, ["token"] = accessToken }, token);
        return NteGlobalIdentity.FromService(response.RootElement, provider, Region);
    }

    internal async Task<JsonDocument> RequestAsync(string path, Dictionary<string, string> fields,
        CancellationToken token)
    {
        fields = new(fields, StringComparer.Ordinal)
        {
            ["appId"] = Region.AppId,
            ["timestamp"] = DateTimeOffset.UtcNow.ToUnixTimeSeconds().ToString(CultureInfo.InvariantCulture)
        };
        fields["sign"] = Sign(fields, configuration.ApplicationKey);
        using HttpRequestMessage request = new(HttpMethod.Post, new Uri(configuration.Service, path));
        request.Headers.TryAddWithoutValidation("X-Client-Data", ClientData);
        request.Content = new FormUrlEncodedContent(fields);
        using HttpResponseMessage response =
            await http.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, token);
        if (response.StatusCode != HttpStatusCode.OK) throw new NteGlobalAuthenticationException("ServiceUnavailable");
        await using Stream stream = await response.Content.ReadAsStreamAsync(token);
        using MemoryStream bytes = new();
        byte[] buffer = new byte[8192];
        try
        {
            int count;
            while ((count = await stream.ReadAsync(buffer, token)) > 0)
            {
                if (bytes.Length + count > 1024 * 1024)
                    throw new InvalidDataException("Global service response size bound.");
                await bytes.WriteAsync(buffer.AsMemory(0, count), token);
            }

            return ParseOwned(bytes.GetBuffer().AsSpan(0, (int)bytes.Length));
        }
        finally
        {
            CryptographicOperations.ZeroMemory(buffer);
            CryptographicOperations.ZeroMemory(bytes.GetBuffer());
            fields.Clear();
        }
    }

    private static JsonDocument ParseOwned(ReadOnlySpan<byte> bytes)
    {
        Utf8JsonReader reader = new(bytes);
        return JsonDocument.ParseValue(ref reader);
    }

    internal static string Sign(IReadOnlyDictionary<string, string> fields, ReadOnlySpan<byte> key)
    {
        byte[] values = Encoding.UTF8.GetBytes(string.Concat(fields.Where(p => p.Key is not ("appKey" or "sign"))
            .OrderBy(p => p.Key, StringComparer.Ordinal).Select(p => p.Value)));
        byte[] joined = new byte[values.Length + key.Length];
        values.CopyTo(joined, 0);
        key.CopyTo(joined.AsSpan(values.Length));
        try
        {
            return Convert.ToHexStringLower(MD5.HashData(joined));
        }
        finally
        {
            CryptographicOperations.ZeroMemory(values);
            CryptographicOperations.ZeroMemory(joined);
        }
    }

    internal static void RequireSuccess(JsonElement response)
    {
        if (!response.TryGetProperty("code", out JsonElement code) || !code.TryGetInt32(out int value))
            throw new InvalidDataException("Unknown global response.");
        if (value != 0) throw new NteGlobalAuthenticationException("ServiceRejected", value);
    }

    internal static JsonElement RequireResult(JsonElement response)
    {
        RequireSuccess(response);
        return response.TryGetProperty("result", out JsonElement result) && result.ValueKind == JsonValueKind.Object
            ? result
            : throw new InvalidDataException("Missing global service result.");
    }

    private void RequireReady()
    {
        if (!Initialized) throw new InvalidOperationException("Initialize global service first.");
    }

    private static void ValidateEmail(string value)
    {
        if (value.Length is < 3 or > 128 || !MailAddress.TryCreate(value.Trim(), out MailAddress? address) ||
            address.Address != value.Trim())
            throw new NteGlobalAuthenticationException("InvalidEmail");
    }

    public void Dispose()
    {
        initialization?.Dispose();
        if (ownsHttp) http.Dispose();
    }

    public override string ToString() => "Global authentication [identity omitted]";
}
