using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

namespace Nte.Login;

internal sealed record NtePhoneCountry(string CountryCode, string AreaName)
{
    public string DisplayName => AreaName + " (" + CountryCode + ")";
    internal static IReadOnlyList<NtePhoneCountry> Preview { get; } =
        [new("+886", "臺灣"), new("+852", "香港"), new("+853", "澳門")];
}

internal sealed partial class NteGlobalAuthentication
{
    internal bool SupportsPhone => Region.HostRegion == NteHostRegion.Taiwan;
    internal IReadOnlyList<NtePhoneCountry> PhoneCountries { get; private set; } = [];
    internal string DefaultCountryCode { get; private set; } = "+886";

    internal async Task LoadPhoneCountriesAsync(CancellationToken token)
    {
        RequirePhoneMethod();
        using JsonDocument reply = await RequestAsync("/c/api/phone/countryCode/list", [], token);
        JsonElement result = RequireResult(reply.RootElement);
        if (!result.TryGetProperty("codes", out JsonElement codes) || codes.ValueKind != JsonValueKind.Array || codes.GetArrayLength() is < 1 or > 64)
            throw new InvalidDataException("Invalid public phone country directory.");
        List<NtePhoneCountry> countries = [];
        HashSet<string> seen = [];
        foreach (JsonElement item in codes.EnumerateArray())
        {
            string code = item.GetProperty("countryCode").GetString() ?? "";
            string name = item.GetProperty("areaName").GetString() ?? "";
            if (!Regex.IsMatch(code, @"\A\+[1-9][0-9]{0,3}\z") || NteGlobalProfile.TextValue(name, 80) is not { } label || !seen.Add(code))
                throw new InvalidDataException("Invalid public phone country entry.");
            countries.Add(new(code, label));
        }
        string? preferred = initialization!.RootElement.TryGetProperty("defaultCountryCode", out JsonElement value) && value.ValueKind == JsonValueKind.String
            ? value.GetString() : null;
        DefaultCountryCode = countries.Any(item => item.CountryCode == preferred) ? preferred! : countries[0].CountryCode;
        PhoneCountries = countries;
    }

    internal async Task SendPhoneCaptchaAsync(string countryCode, string phone, CancellationToken token)
    {
        Dictionary<string, string> fields = PhoneFields(countryCode, phone);
        if (CaptchaCooldown > TimeSpan.Zero) throw new NteGlobalAuthenticationException("SmsCooldown");
        nextCaptcha = DateTimeOffset.UtcNow.AddSeconds(60);
        using JsonDocument reply = await RequestAsync("/c/api/phone/get/sms/captcha", fields, token);
        RequireSuccess(reply.RootElement);
    }

    internal async Task<NteGlobalIdentity> LoginPhoneAsync(string countryCode, string phone, string secret, bool password, CancellationToken token)
    {
        Dictionary<string, string> fields = PhoneFields(countryCode, phone);
        if (password)
        {
            if (secret.Length is < 1 or > 256 || secret.Contains('\0')) throw new NteGlobalAuthenticationException("InvalidSecret");
            fields["password"] = EncryptPassword(secret);
        }
        else
        {
            if (!Regex.IsMatch(secret, @"\A[0-9]{4,8}\z")) throw new NteGlobalAuthenticationException("InvalidSmsCode");
            fields["smsCode"] = secret;
        }
        using JsonDocument response = await RequestAsync(password ? "/c/api/phone/login/password" : "/c/api/phone/login", fields, token);
        return NteGlobalIdentity.FromService(response.RootElement, 7, Region).WithProfile(NteGlobalProfile.FromPhone(countryCode, phone));
    }

    private Dictionary<string, string> PhoneFields(string countryCode, string phone)
    {
        RequirePhoneMethod();
        if (!PhoneCountries.Any(item => item.CountryCode == countryCode) || !Regex.IsMatch(phone, @"\A[0-9]{6,15}\z"))
            throw new NteGlobalAuthenticationException("InvalidOverseasPhone");
        return new() { ["countryCode"] = countryCode, ["phoneNumber"] = configuration.EncodeEmail(phone) };
    }

    private void RequirePhoneMethod()
    {
        if (!SupportsPhone) throw new NteGlobalAuthenticationException("LoginMethodUnavailable");
        RequireReady();
        if (!initialization!.RootElement.TryGetProperty("loginTypes", out JsonElement types) || types.ValueKind != JsonValueKind.Array ||
            !types.EnumerateArray().Any(value => value.TryGetInt32(out int type) && type == 7))
            throw new NteGlobalAuthenticationException("LoginMethodUnavailable");
    }

    private string EncryptPassword(string secret)
    {
        byte[] publicKey = Convert.FromBase64String(initialization!.RootElement.GetProperty("clientPublicKey").GetString()!);
        byte[] plain = Encoding.UTF8.GetBytes(secret);
        try
        {
            using RSA rsa = RSA.Create();
            rsa.ImportSubjectPublicKeyInfo(publicKey, out int used);
            if (used != publicKey.Length || rsa.KeySize < 1024) throw new InvalidDataException("Unsupported password public key.");
            byte[] cipher = rsa.Encrypt(plain, RSAEncryptionPadding.Pkcs1);
            try { return Convert.ToBase64String(cipher); }
            finally { CryptographicOperations.ZeroMemory(cipher); }
        }
        finally { CryptographicOperations.ZeroMemory(plain); CryptographicOperations.ZeroMemory(publicKey); }
    }
}
