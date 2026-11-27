using System;
using System.IO;
using System.Text.Json;

namespace Nte.Login;

internal static class NteLoginPreferences
{
    internal static string DefaultPath => Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "TouchUILaunch", "NTE", "login.json");

    internal static string GlobalPath => Path.Combine(Path.GetDirectoryName(DefaultPath)!, "Global", "login.json");

    internal static string GetPath(NteHostRegion region) => region switch
    {
        NteHostRegion.CN => DefaultPath, NteHostRegion.Global => GlobalPath,
        NteHostRegion.Taiwan => Path.Combine(Path.GetDirectoryName(DefaultPath)!, "Taiwan", "login.json"),
        _ => throw new ArgumentException("Unknown login region.", nameof(region))
    };

    internal static string GetPath(bool global) => global ? GlobalPath : DefaultPath;

    internal static bool ReadExperimentalHello(string path)
    {
        if (!File.Exists(path)) return false;
        if (new FileInfo(path).Length is < 1 or > 4096) throw new InvalidDataException("Invalid login preferences.");
        using JsonDocument document = JsonDocument.Parse(File.ReadAllBytes(path));
        JsonElement root = document.RootElement;
        if (root.ValueKind != JsonValueKind.Object || !root.TryGetProperty("format", out var format) || format.ValueKind != JsonValueKind.Number || !format.TryGetInt32(out int number) || number != 1 ||
            !root.TryGetProperty("experimentalWindowsHello", out var value) || value.ValueKind is not (JsonValueKind.True or JsonValueKind.False))
            throw new InvalidDataException("Invalid login preferences contract.");
        return value.GetBoolean();
    }

    internal static void WriteExperimentalHello(string path, bool enabled)
    {
        path = Path.GetFullPath(path); Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        string temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            using (FileStream stream = new(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096, FileOptions.WriteThrough))
            {
                using Utf8JsonWriter writer = new(stream); writer.WriteStartObject(); writer.WriteNumber("format", 1);
                writer.WriteBoolean("experimentalWindowsHello", enabled); writer.WriteEndObject(); writer.Flush(); stream.Flush(true);
            }
            File.Move(temporary, path, true);
        }
        finally { if (File.Exists(temporary)) File.Delete(temporary); }
    }
}
