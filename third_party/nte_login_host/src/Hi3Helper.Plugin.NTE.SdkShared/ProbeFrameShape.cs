using System.Text;
using System.Text.Json;
using System.Text.RegularExpressions;

internal static class ProbeFrameShape
{
    public static HashSet<string> LoadPublicMetadata(params string[] modules)
    {
        HashSet<string> values = new(StringComparer.OrdinalIgnoreCase);
        foreach (string module in modules)
            foreach (Encoding encoding in new[] { Encoding.ASCII, Encoding.Unicode })
            {
                string text = encoding.GetString(File.ReadAllBytes(module));
                foreach (Match match in Regex.Matches(text, @"[A-Za-z0-9_./-]{2,96}\.(?:config|json|ini)\b|\b\d{1,3}(?:\.\d{1,5}){2,3}\b"))
                {
                    string value = match.Value;
                    // Only exact static vendor literals can be revealed below;
                    // also permit their public basename for relative SDK paths.
                    values.Add(value);
                    values.Add(value[(value.LastIndexOf('/') + 1)..]);
                }
            }
        return values;
    }

    public static HashSet<string> LoadPublicSelectors(params string[] modules)
    {
        HashSet<string> names = new(StringComparer.Ordinal);
        foreach (string module in modules)
        {
            byte[] data = File.ReadAllBytes(module);
            foreach (Encoding encoding in new[] { Encoding.ASCII, Encoding.Unicode })
                foreach (Match match in Regex.Matches(encoding.GetString(data), @"[A-Za-z_][A-Za-z0-9_]{2,63}"))
                    if (Regex.IsMatch(match.Value, @"\A(?:get|set|open|show|query|init|request|report|sdk|game|client|device|check)[A-Za-z_]{1,50}\z", RegexOptions.IgnoreCase))
                        names.Add(match.Value);
        }
        return names;
    }

    public static object Inspect(ReadOnlySpan<byte> frame, int clientId, IReadOnlySet<string> selectors,
        IReadOnlySet<string> publicMetadata, string? application)
    {
        // Only the public packet structure is recorded. The SDK XORs request
        // bodies with the client's PID byte; never retain the decoded body.
        byte[] body = frame[12..].ToArray();
        for (int index = 0; index < body.Length; index++) body[index] ^= (byte)clientId;
        List<object> structures = [];
        uint operation = BitConverter.ToUInt32(frame[4..]);
        if (operation == 40 && frame.Length == 0x214)
            structures.Add(new { type = "game-query", queryType = BitConverter.ToInt32(body), contextId = BitConverter.ToInt32(body, 0x204) });
        if (operation == 0 && frame.Length == 1244)
        {
            // This register-client packet carries a callback HWND and public
            // application identifiers. Report lengths only for wide fields.
            string[] fields = [WideField(4, 128), WideField(0x104, 128), WideField(0x204, 260), WideField(0x40C, 32)];
            structures.Add(new { type = "register-client", callbackWindow = BitConverter.ToUInt32(body),
                fieldCharacters = fields.Select(field => field.Length).ToArray(),
                applicationMatchesInitialization = application != null && fields[0] == application,
                publicConfig = publicMetadata.Contains(fields[2]) && Regex.IsMatch(fields[2], @"\.(?:config|json|ini)\z", RegexOptions.IgnoreCase) ? fields[2] : null,
                publicSdkVersion = publicMetadata.Contains(fields[3]) && Regex.IsMatch(fields[3], @"\A\d+(?:\.\d+){2,3}\z") ? fields[3] : null });
        }
        foreach (int offset in new[] { 0, 4, 8 })
        {
            if (offset >= body.Length) continue;
            int end = Array.IndexOf(body, (byte)0, offset);
            if (end < 0) end = body.Length;
            string text = Encoding.UTF8.GetString(body.AsSpan(offset, end - offset));
            if (!text.StartsWith('{')) continue;
            try
            {
                using JsonDocument json = JsonDocument.Parse(text);
                string[] properties = json.RootElement.EnumerateObject().Select(property => property.Name)
                    .Where(name => Regex.IsMatch(name, @"\A[A-Za-z_][A-Za-z0-9_]{0,39}\z"))
                    .Distinct().Order().Take(32).ToArray();
                // A selector is included only when its exact text is a public
                // API-like literal in the installed vendor binaries. Arbitrary
                // key/value fields, account IDs and tokens remain omitted.
                Dictionary<string, string> publicSelectors = new();
                foreach (string key in new[] { "key", "hint", "method", "action" })
                    if (json.RootElement.TryGetProperty(key, out JsonElement selector) && selector.ValueKind == JsonValueKind.String &&
                        selector.GetString() is { } value && selectors.Contains(value)) publicSelectors[key] = value;
                structures.Add(new { offset, type = "json-object", properties, publicSelectors });
            }
            catch (JsonException) { /* Not a complete JSON object. Do not record it. */ }
        }
        Array.Clear(body);
        return new { bodyBytes = frame.Length - 12, structures };

        string WideField(int offset, int capacity)
        {
            string field = Encoding.Unicode.GetString(body.AsSpan(offset, capacity * 2));
            int end = field.IndexOf('\0');
            return end < 0 ? field : field[..end];
        }
    }
}
