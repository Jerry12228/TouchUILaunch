using System.Globalization;
using System.Net;
using System.Net.Mail;
using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Nte.Login;

// The installed image is read as data only. No vendor executable or private
// function is loaded. Static application material never enters diagnostics.
internal sealed class NteOverseasApplicationConfiguration : IDisposable
{
    internal NteOverseasRegion Region { get; }
    internal byte[] ApplicationKey { get; }
    internal string PackageName { get; }
    internal Uri Service { get; }
    internal string GameArea { get; }
    internal IReadOnlyDictionary<string, string> GameServices { get; }
    internal NteOverseasApplicationConfiguration(byte[] key, string packageName, Uri service, string gameArea,
        IReadOnlyDictionary<string, string>? gameServices = null, NteOverseasRegion? region = null)
    {
        Region = region ?? NteOverseasRegion.Global;
        ApplicationKey = key.ToArray(); PackageName = packageName; Service = service; GameArea = gameArea;
        GameServices = gameServices?.ToDictionary(p => p.Key, p => p.Value, StringComparer.Ordinal) ?? [];
    }

    internal static NteOverseasApplicationConfiguration Load(string root, NteHostRegion hostRegion = NteHostRegion.Global)
    {
        NteOverseasRegion region = NteOverseasRegion.ForInstallation(hostRegion, root);
        string platform = Path.Combine(root, region.LauncherDirectory);
        byte[] image = File.ReadAllBytes(Path.Combine(platform, region.Installation.LauncherExecutable));
        byte[]? key = null, appKey = null, resource = null;
        try
        {
            if (Convert.ToHexString(SHA256.HashData(image)) != region.ImageSha256)
                throw new NteGlobalLaunchException("GlobalSdkUnsupported");
            using PEReader pe = new(new MemoryStream(image, false));
            int keyRva = region.Sdk.ConfigurationKeyRva;
            var section = pe.PEHeaders.SectionHeaders.Single(s => keyRva >= s.VirtualAddress && keyRva + 24 <= s.VirtualAddress + s.SizeOfRawData);
            key = Convert.FromBase64String(Encoding.ASCII.GetString(image, section.PointerToRawData + keyRva - section.VirtualAddress, 24));
            resource = ProbeBootConfig.ReadStaticResource(Path.Combine(platform, "ResData", region.AppId + ".dat"), "/" + region.AppId + "/globalsdkconfig.json");
            using JsonDocument json = JsonDocument.Parse(resource);
            JsonElement config = json.RootElement.GetProperty("base");
            string Decode(string name)
            {
                byte[] cipher = Convert.FromBase64String(config.GetProperty(name).GetString()!);
                byte[]? plain = null;
                try { using Aes aes = Aes.Create(); aes.Key = key; plain = aes.DecryptEcb(cipher, PaddingMode.PKCS7); return Encoding.UTF8.GetString(plain); }
                finally { CryptographicOperations.ZeroMemory(cipher); if (plain != null) CryptographicOperations.ZeroMemory(plain); }
            }
            if (Decode("APPID") != region.AppId || new Uri(Decode("SdkDomain")) != region.Service || Decode("PackageName") != region.PackageName)
                throw new InvalidDataException("Unexpected global public configuration.");
            appKey = Encoding.UTF8.GetBytes(Decode("APPKEY"));
            if (appKey.Length != 40) throw new InvalidDataException("Unsupported global application configuration.");
            string area = Decode("area");
            if (area != region.Area) throw new InvalidDataException("Unsupported overseas game area.");
            Dictionary<string, string> services = new()
            {
                ["chatCoreDomain"] = Decode("chatSdk_coreDomain"),
                ["chatTCPDomain"] = Decode("imSdk_tcpDomain"),
                ["chatIMApiDomain"] = Decode("imSdk_imapiDomain")
            };
            if (services.Values.Any(value => string.IsNullOrWhiteSpace(value) || value.Length > 512 || value.Contains('\0')))
                throw new InvalidDataException("Unsupported global game service configuration.");
            return new(appKey, region.PackageName, region.Service, area, services, region);
        }
        finally
        {
            CryptographicOperations.ZeroMemory(image);
            if (key != null) CryptographicOperations.ZeroMemory(key);
            if (appKey != null) CryptographicOperations.ZeroMemory(appKey);
            if (resource != null) CryptographicOperations.ZeroMemory(resource);
        }
    }
    internal string DecodeServiceValue(string value)
    {
        byte[] cipher = Convert.FromBase64String(value);
        byte[]? plain = null;
        try { using Aes aes = Aes.Create(); aes.Key = ApplicationKey.AsSpan(0, 16).ToArray(); plain = aes.DecryptEcb(cipher, PaddingMode.PKCS7); return Encoding.UTF8.GetString(plain); }
        finally { CryptographicOperations.ZeroMemory(cipher); if (plain != null) CryptographicOperations.ZeroMemory(plain); }
    }
    internal string EncodeEmail(string email)
    {
        byte[] plain = Encoding.UTF8.GetBytes(email.Trim());
        try
        {
            using Aes aes = Aes.Create(); aes.Key = ApplicationKey.AsSpan(0, 16).ToArray();
            byte[] cipher = aes.EncryptEcb(plain, PaddingMode.PKCS7);
            try { return Convert.ToBase64String(cipher); }
            finally { CryptographicOperations.ZeroMemory(cipher); }
        }
        finally { CryptographicOperations.ZeroMemory(plain); }
    }
    public void Dispose() => CryptographicOperations.ZeroMemory(ApplicationKey);
    public override string ToString() => "Global application configuration [key omitted]";
}
