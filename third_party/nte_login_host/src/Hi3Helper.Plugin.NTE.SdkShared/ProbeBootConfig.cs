using System.Buffers.Binary;
using System.IO.Compression;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;
using Nte.Login;

// Reads only the vendor's static resource and this client's initialization
// packet. The app key and decoded configuration stay in this process; the
// report contains field names, types, and narrowly allowed public metadata.
internal static class ProbeBootConfig
{
    public static object Inspect(NteSdkRegistration registration, string? application, string root,
        Func<JsonElement, object>? inspectConfiguration = null,
        Func<JsonElement, ReadOnlyMemory<byte>, object>? inspectWithApplicationKey = null)
    {
        byte[] key = [];
        byte[] applicationKey = [];
        byte[]? plaintext = null;
        try
        {
            if (application == null || application != registration.Protocol.Application)
                throw new InvalidDataException("Not a validated client registration.");
            applicationKey = registration.ReadCnApplicationKey();
            key = applicationKey.AsSpan(16, 16).ToArray();
            string config = NteSdkRegistration.CnConfigurationName;
            // Fixed-image static path: AES wrapper RVA 0x47C010 takes the
            // application's slice starting at character 16, decodes Base64, then decrypts
            // each 16-byte block with the AES wrapper at RVA 0x47BEB0.
            string resource = Path.Combine(root, NteRegionDefinition.CN.LauncherDirectory, "ResData", application + ".dat");
            byte[] encoded = ReadStaticResource(resource, "/" + application + "/main/" + config);
            byte[] ciphertext = Convert.FromBase64String(Encoding.ASCII.GetString(encoded));
            using Aes aes = Aes.Create();
            aes.Key = key;
            plaintext = aes.DecryptEcb(ciphertext, PaddingMode.PKCS7);
            using JsonDocument document = JsonDocument.Parse(plaintext);
            if (document.RootElement.ValueKind != JsonValueKind.Object)
                throw new InvalidDataException("The configuration is not a JSON object.");
            List<object> fields = Describe(document.RootElement);
            return new { decoded = true, config, resourceBytes = encoded.Length, decodedBytes = plaintext.Length,
                algorithm = "AES-128-ECB-PKCS7", keySelection = "application-characters-16-through-31", fields,
                anonymousInitialization = inspectWithApplicationKey != null ? inspectWithApplicationKey(document.RootElement, key)
                    : inspectConfiguration?.Invoke(document.RootElement), error = (string?)null };
        }
        catch (Exception ex) when (ex is IOException or CryptographicException or JsonException or FormatException or OverflowException)
        {
            // No exception message may include a key, configuration or account.
            return new { decoded = false, error = ex.GetType().Name };
        }
        finally
        {
            CryptographicOperations.ZeroMemory(applicationKey);
            CryptographicOperations.ZeroMemory(key);
            if (plaintext != null) CryptographicOperations.ZeroMemory(plaintext);
        }
    }

    private static List<object> Describe(JsonElement root)
    {
        List<object> fields = [];
        Visit(root, "", 0);
        return fields;

        void Visit(JsonElement element, string parent, int depth)
        {
            if (depth > 6 || element.ValueKind != JsonValueKind.Object) return;
            foreach (JsonProperty property in element.EnumerateObject())
            {
                if (!Regex.IsMatch(property.Name, @"\A[A-Za-z_][A-Za-z0-9_]{0,63}\z")) continue;
                string name = parent.Length == 0 ? property.Name : parent + "." + property.Name;
                JsonElement value = property.Value;
                string? text = value.ValueKind == JsonValueKind.String ? value.GetString() : null;
                string? publicValue = null;
                if (property.Name is "appId" or "appid" or "gameId" or "channelId" or "platformId" or "oemId" or "merchantId" ||
                    name is "meta.platform" or "init.channel.channelAppId")
                {
                    string? id = text ?? (value.ValueKind == JsonValueKind.Number ? value.GetRawText() : null);
                    if (id != null && Regex.IsMatch(id, @"\A[0-9]{1,10}\z")) publicValue = id;
                }
                if (name == "meta.version" && text != null && Regex.IsMatch(text, @"\A[0-9]+(?:\.[0-9]+){1,3}\z"))
                    publicValue = text;
                // Service origins are public configuration, but never preserve
                // URL userinfo, query, fragment or any account/session value.
                if (property.Name.EndsWith("Url", StringComparison.OrdinalIgnoreCase) &&
                    Uri.TryCreate(text, UriKind.Absolute, out Uri? uri) && uri.Scheme == "https" &&
                    uri.UserInfo.Length == 0 && uri.Query.Length == 0 && uri.Fragment.Length == 0 &&
                    new[] { "laohu.com", "wanmei.com", "perfectworldgames.com" }.Any(domain =>
                        uri.Host.Equals(domain, StringComparison.OrdinalIgnoreCase) || uri.Host.EndsWith("." + domain, StringComparison.OrdinalIgnoreCase)))
                    publicValue = uri.GetLeftPart(UriPartial.Path);
                fields.Add(new { name, type = value.ValueKind.ToString(), characters = text?.Length, publicValue });
                Visit(value, name, depth + 1);
            }
        }
    }

    internal static byte[] ReadStaticResource(string file, string suffix)
    {
        if (new FileInfo(file).Length > 64 * 1024 * 1024) throw new InvalidDataException("Static resource size bound.");
        byte[] data = File.ReadAllBytes(file);
        if (data.Length < 24 || !data.AsSpan(0, 4).SequenceEqual("qres"u8) || Big32(4) != 3)
            throw new InvalidDataException("Unsupported Qt RCC header.");
        int tree = Big32(8), payload = Big32(12), names = Big32(16);
        int treeEnd = data.Length;
        if (payload > tree) treeEnd = Math.Min(treeEnd, payload);
        if (names > tree) treeEnd = Math.Min(treeEnd, names);
        int count = (treeEnd - tree) / 22;
        if (tree < 24 || payload < 24 || names < 24 || count is <= 0 or > 20000)
            throw new InvalidDataException("Invalid Qt RCC section bounds.");
        Stack<(int Index, string Parent)> pending = new();
        HashSet<int> visited = [];
        pending.Push((0, ""));
        byte[]? match = null;
        while (pending.TryPop(out var entry))
        {
            if ((uint)entry.Index >= (uint)count || !visited.Add(entry.Index))
                throw new InvalidDataException("Invalid or cyclic Qt RCC node.");
            int node = checked(tree + entry.Index * 22);
            int name = checked(names + Big32(node));
            int characters = Big16(name);
            if (characters > 255) throw new InvalidDataException("Qt RCC name bound.");
            string component = Encoding.BigEndianUnicode.GetString(Slice(checked(name + 6), checked(characters * 2)));
            if (component is "." or ".." || component.IndexOfAny(['/', '\\', '\0', ':']) >= 0)
                throw new InvalidDataException("Unsafe Qt RCC path component.");
            string path = entry.Parent + "/" + component;
            int flags = Big16(node + 4);
            if ((flags & 2) != 0)
            {
                int children = Big32(node + 6), first = Big32(node + 10);
                if (children < 0 || first < 0 || first > count || children > count - first)
                    throw new InvalidDataException("Qt RCC child range bound.");
                for (int i = 0; i < children; i++) pending.Push((first + i, path));
                continue;
            }
            if (!path.EndsWith(suffix, StringComparison.Ordinal)) continue;
            if (match != null) throw new InvalidDataException("Ambiguous static SDK configuration.");
            int blob = checked(payload + Big32(node + 10));
            int size = Big32(blob);
            if (size is <= 0 or > 65536 || (flags & ~1) != 0)
                throw new InvalidDataException("Unsupported static SDK resource format.");
            match = Slice(checked(blob + 4), size).ToArray();
            if ((flags & 1) != 0)
            {
                if (match.Length < 4) throw new InvalidDataException("Invalid compressed resource.");
                int expected = checked((int)BinaryPrimitives.ReadUInt32BigEndian(match));
                if (expected is <= 0 or > 65536) throw new InvalidDataException("Compressed SDK config size bound.");
                using MemoryStream input = new(match, 4, match.Length - 4);
                using ZLibStream inflater = new(input, CompressionMode.Decompress);
                byte[] result = new byte[expected + 1];
                int read = inflater.ReadAtLeast(result, result.Length, throwOnEndOfStream: false);
                if (read != expected) throw new InvalidDataException("Compressed SDK config length mismatch.");
                match = result[..expected];
            }
        }
        return match ?? throw new FileNotFoundException("Static SDK configuration resource is missing.");

        int Big32(int offset) => checked((int)BinaryPrimitives.ReadUInt32BigEndian(Slice(offset, 4)));
        int Big16(int offset) => BinaryPrimitives.ReadUInt16BigEndian(Slice(offset, 2));
        ReadOnlySpan<byte> Slice(int offset, int length)
        {
            if (offset < 0 || length < 0 || offset > data.Length - length)
                throw new InvalidDataException("Qt RCC read exceeds file bounds.");
            return data.AsSpan(offset, length);
        }
    }
}
