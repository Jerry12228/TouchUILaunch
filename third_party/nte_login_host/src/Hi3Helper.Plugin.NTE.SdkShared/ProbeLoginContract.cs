using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

// Explicit diagnostic mode only: compare parameter validation with no phone,
// captcha, password, UID or token. Never accept an identity from this response.
internal static class ProbeLoginContract
{
    internal static async Task<object> RunAsync(JsonElement config, string installedVersion, CancellationToken token)
    {
        using HttpClient client = new(new HttpClientHandler { AllowAutoRedirect = false, UseCookies = false,
            AutomaticDecompression = DecompressionMethods.All }) { Timeout = TimeSpan.FromSeconds(15), MaxResponseContentBufferSize = 65536 };
        List<object> results = [];
        string device = Guid.NewGuid().ToString("N");
        foreach (int profile in new[] { 0, 1, 2 })
        {
            bool pcParameters = profile == 1;
            string label = profile == 2 ? "native-channel-anonymous-init" : pcParameters ? "pc-type-and-installed-version" : "previous-candidate";
            SortedDictionary<string, string> values = profile == 2
                ? ProbeAuthenticationClient.BuildChannelParameters(config, installedVersion, device)
                : BuildParameters(config, installedVersion, pcParameters, device);
            byte[] responseBytes = [];
            try
            {
                Uri endpoint = profile == 2 ? ProbeAnonymousInit.Endpoint : new("https://user.laohu.com/openApi/sms/new/login");
                using HttpRequestMessage request = new(HttpMethod.Post, endpoint)
                    { Content = new FormUrlEncodedContent(values) };
                using HttpResponseMessage response = await client.SendAsync(request, token).ConfigureAwait(false);
                responseBytes = await response.Content.ReadAsByteArrayAsync(token).ConfigureAwait(false);
                using JsonDocument document = JsonDocument.Parse(responseBytes);
                JsonElement root = document.RootElement;
                int? code = root.ValueKind == JsonValueKind.Object && root.TryGetProperty("code", out JsonElement number) &&
                    number.TryGetInt32(out int value) ? value : null;
                string? message = null;
                foreach (string name in new[] { "message", "msg" })
                    if (root.TryGetProperty(name, out JsonElement field) && field.ValueKind == JsonValueKind.String)
                        message = SafeExplanation(field.GetString());
                results.Add(new { profile = label, endpoint = endpoint.AbsoluteUri,
                    httpStatus = (int)response.StatusCode, serverCode = code, publicValidationMessage = message,
                    requestFields = values.Keys.ToArray(), accountDataSupplied = false });
            }
            catch (Exception ex) when (ex is HttpRequestException or IOException or JsonException or OperationCanceledException)
            {
                results.Add(new { profile = label, error = ex.GetType().Name,
                    accountDataSupplied = false });
            }
            finally { values.Clear(); CryptographicOperations.ZeroMemory(responseBytes); }
        }
        return new { endpoint = "https://user.laohu.com/openApi/sms/new/login", results,
            accountDataSupplied = false, smsSent = false, loginAttempted = false, identityAccepted = false };
    }

    internal static SortedDictionary<string, string> BuildParameters(JsonElement config, string installedVersion, bool pcParameters, string device)
    {
        if (!Regex.IsMatch(installedVersion, @"\A[0-9]+(?:\.[0-9]+){1,3}\z")) throw new InvalidDataException("Missing public installed version.");
        SortedDictionary<string, string> values = ProbeAnonymousInit.BuildParameters(config, DateTimeOffset.UtcNow, device);
        values.Remove("sign");
        values["type"] = "18"; values["areaCodeId"] = "1"; values["deviceName"] = "WindowsPc";
        foreach (string name in new[] { "version", "idfa", "adm", "mac" }) values[name] = "";
        values["t"] = DateTimeOffset.UtcNow.ToUnixTimeMilliseconds().ToString(System.Globalization.CultureInfo.InvariantCulture);
        if (pcParameters) { values["deviceType"] = "WindowsPc"; values["version"] = installedVersion; }
        string key = config.GetProperty("init").GetProperty("channel").GetProperty("channelAppKey").GetString()!;
        byte[] signed = Encoding.UTF8.GetBytes(string.Concat(values.Values) + key);
        try { values["sign"] = Convert.ToHexStringLower(MD5.HashData(signed)); }
        finally { CryptographicOperations.ZeroMemory(signed); }
        return values;
    }

    internal static string? SafeExplanation(string? text)
    {
        if (string.IsNullOrWhiteSpace(text) || text.Length > 256) return null;
        // A service error can echo input. This function also protects the local
        // UI; only the credential-free diagnostic persists its sanitized text.
        text = Regex.Replace(text, @"[\p{Cc}\p{Cf}]", " ");
        text = Regex.Replace(text, @"https?://\S+|\S+@\S+|[0-9]{4,}|[A-Za-z0-9_+/=\-]{24,}", "[omitted]");
        return text;
    }
}
