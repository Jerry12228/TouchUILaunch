using System.Text;
using System.Text.RegularExpressions;

internal static class ProbeSdkLog
{
    // Keep categories only. SDK log lines may include host paths or arbitrary
    // parameters; neither their text nor raw files are retained in a report.
    private static readonly string[] Markers = [
        "Get EnvironmentVariable Failed", "ID does not match", "Get notify msg failed",
        "COREClientLaunched check begin", "Client Installed And Launched",
        "OpenProcess", "OpenEvent", "OpenMutex", "OpenFileMapping", "MapViewOfFile",
        "anchor_w", "anchor_v", "Init failed", "Init fail", "Init Finished:", "Check Client", "SharedMem",
        "no platform", "not start from pgp", "Get AppInfo result", "ClientInfo result", "GetStartInfo result="
    ];

    public static object[] CollectAndRemove(string root)
    {
        List<object> result = [];
        foreach (string file in Directory.EnumerateFiles(root))
        {
            byte[] bytes = File.ReadAllBytes(file);
            string text = bytes.Length > 2 && (bytes[0] == 0xff && bytes[1] == 0xfe || bytes[1] == 0)
                ? Encoding.Unicode.GetString(bytes) : Encoding.UTF8.GetString(bytes);
            result.AddRange(Summarize(text));
            Array.Clear(bytes);
            File.Delete(file);
        }
        Directory.Delete(root);
        return result.ToArray();
    }

    internal static object[] Summarize(string text)
    {
        List<object> result = [];
        foreach (string line in text.Split('\n'))
        {
            // This fixed log message prints an account ID after the pointer.
            // Retain only whether the callback exists; neither value escapes.
            Match login = Regex.Match(line, @"loginSuccess Begin,\s*(?:0x)?([0-9a-fA-F]{1,16}),\s*uid:");
            if (login.Success)
                result.Add(new { category = "login-result-received", callbackRegistered = login.Groups[1].Value.Any(c => c != '0') });
            if (line.Contains("loginSuccess End", StringComparison.Ordinal))
                result.Add(new { category = "login-result-handled" });
            foreach (string marker in Markers)
                if (line.Contains(marker, StringComparison.OrdinalIgnoreCase))
                {
                    string[] codes = Regex.Matches(line, @"(?i)\b0x[0-9a-f]{1,8}\b").Select(match => match.Value).ToArray();
                    Match numeric = Regex.Match(line, @"(?i)(?:result|errcode|failed|fail)\s*[:=]\s*(-?\d{1,10}|[a-f0-9]{8})\b");
                    result.Add(new { category = marker, codes, result = numeric.Success ? numeric.Groups[1].Value : null });
                }
        }
        return result.ToArray();
    }
}
