using System.Globalization;
using System.Net;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

// A single, explicit anonymous initialization request. It never sends SMS,
// attempts login, reads an account store, follows redirects, or reuses cookies.
internal static class ProbeAnonymousInit
{
    internal static readonly Uri Endpoint = new("https://user.laohu.com/m/newApi/initConfig");

    public static async Task<object> RunAsync(JsonElement config, CancellationToken token)
    {
        using HttpClientHandler handler = new() { AllowAutoRedirect = false, UseCookies = false, AutomaticDecompression = DecompressionMethods.All };
        using HttpClient client = new(handler) { Timeout = TimeSpan.FromSeconds(15) };
        return await SendAsync(client, config, token).ConfigureAwait(false);
    }

    internal static async Task<object> SendAsync(HttpClient client, JsonElement config, CancellationToken token)
    {
        byte[]? responseBytes = null;
        int? status = null;
        try
        {
            SortedDictionary<string, string> parameters = BuildParameters(config, DateTimeOffset.UtcNow, Guid.NewGuid().ToString("N"));
            using HttpRequestMessage request = new(HttpMethod.Post, Endpoint) { Content = new FormUrlEncodedContent(parameters) };
            request.Headers.Accept.ParseAdd("application/json");
            request.Headers.UserAgent.ParseAdd("TouchUILaunch-NTE-SdkProbe/0.4.2");
            using HttpResponseMessage response = await client.SendAsync(request, HttpCompletionOption.ResponseHeadersRead, token).ConfigureAwait(false);
            status = (int)response.StatusCode;
            if (response.Content.Headers.ContentLength > 1024 * 1024) throw new InvalidDataException("Anonymous response size bound.");
            await using Stream input = await response.Content.ReadAsStreamAsync(token).ConfigureAwait(false);
            using MemoryStream output = new();
            byte[] buffer = new byte[8192];
            try
            {
                int count;
                while ((count = await input.ReadAsync(buffer, token).ConfigureAwait(false)) != 0)
                {
                    if (output.Length + count > 1024 * 1024) throw new InvalidDataException("Anonymous response size bound.");
                    output.Write(buffer, 0, count);
                }
                responseBytes = output.ToArray();
            }
            finally { CryptographicOperations.ZeroMemory(buffer); }
            using JsonDocument json = JsonDocument.Parse(responseBytes);
            JsonElement root = json.RootElement;
            int? code = root.TryGetProperty("code", out JsonElement codeValue) && codeValue.TryGetInt32(out int number) ? number : null;
            bool hasResult = root.TryGetProperty("result", out JsonElement result) && result.ValueKind == JsonValueKind.Object;
            int[] loginOptions = hasResult && result.TryGetProperty("loginOptionConfig", out JsonElement options) && options.ValueKind == JsonValueKind.Array
                ? options.EnumerateArray().Where(value => value.ValueKind == JsonValueKind.Number && value.TryGetInt32(out int id) && id is >= 0 and <= 10000)
                    .Select(value => value.GetInt32()).Take(32).ToArray() : [];
            string[] resultFields = hasResult ? Names(result) : [];
            return new { attempted = true, endpoint = Endpoint.AbsoluteUri, httpStatus = status, serverCode = code,
                initialized = response.IsSuccessStatusCode && code == 0 && hasResult,
                requestFields = parameters.Keys.ToArray(), resultFields, loginOptions,
                accountRequestSent = false, error = (string?)null };
        }
        catch (Exception ex) when (ex is HttpRequestException or IOException or JsonException or InvalidOperationException or OperationCanceledException or ArgumentException or KeyNotFoundException)
        {
            return new { attempted = status != null || ex is HttpRequestException or OperationCanceledException,
                endpoint = Endpoint.AbsoluteUri, httpStatus = status, initialized = false,
                accountRequestSent = false, error = ex.GetType().Name };
        }
        finally { if (responseBytes != null) CryptographicOperations.ZeroMemory(responseBytes); }
    }

    internal static SortedDictionary<string, string> BuildParameters(JsonElement config, DateTimeOffset time, string deviceId)
    {
        JsonElement channel = config.GetProperty("init").GetProperty("channel");
        JsonElement meta = config.GetProperty("meta");
        string appId = StringValue(channel, "channelAppId");
        string appKey = StringValue(channel, "channelAppKey");
        if (!Regex.IsMatch(appId, @"\A[0-9]{1,10}\z") || !Regex.IsMatch(appKey, @"\A[A-Za-z0-9]{32}\z") ||
            !Regex.IsMatch(deviceId, @"\A[0-9a-f]{32}\z"))
            throw new InvalidDataException("Unexpected static channel metadata.");
        // Names and backend version match the fixed PC request builder at
        // NTEGame RVA 0x3E90B0. The probe owns a fresh anonymous device ID;
        // it does not copy the official launcher's device or account identity.
        SortedDictionary<string, string> values = new(StringComparer.Ordinal)
        {
            ["appId"] = appId,
            ["appVersion"] = StringValue(meta, "version"),
            ["packageName"] = StringValue(meta, "packageName"),
            ["channelId"] = StringValue(meta, "channelId"),
            ["deviceId"] = deviceId,
            ["deviceModel"] = "WindowsPc",
            ["deviceSys"] = "WindowsPc",
            ["deviceType"] = Environment.OSVersion.VersionString + " 64bit",
            ["sdkVersion"] = Nte.Login.NteCnSdkCompatibility.ServiceSdkVersion,
            ["t"] = time.ToUnixTimeSeconds().ToString(CultureInfo.InvariantCulture)
        };
        byte[] input = Encoding.UTF8.GetBytes(string.Concat(values.Values) + appKey);
        try { values["sign"] = Convert.ToHexStringLower(MD5.HashData(input)); }
        finally { CryptographicOperations.ZeroMemory(input); }
        return values;
    }

    private static string StringValue(JsonElement parent, string name)
    {
        JsonElement value = parent.GetProperty(name);
        string? result = value.ValueKind == JsonValueKind.String ? value.GetString()
            : value.ValueKind == JsonValueKind.Number ? value.GetRawText() : null;
        if (result == null || result.Length is 0 or > 128) throw new InvalidDataException("Unsupported static metadata value.");
        return result;
    }

    private static string[] Names(JsonElement value) => value.EnumerateObject().Select(property => property.Name)
        .Where(name => Regex.IsMatch(name, @"\A[A-Za-z_][A-Za-z0-9_]{0,63}\z")).Distinct().Order().Take(64).ToArray();
}
