using System;
using System.IO;
using System.Text.Json;

namespace Nte.Login;

// Shared by the settings page and the login host. Both entry points write the
// same per-region file, preserving the other launch choices.
internal static class NteLaunchPreferences
{
    internal static string GetPath(NteHostRegion region) => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "TouchUILaunch", "NTE", region switch
        {
            NteHostRegion.CN => "NteCN", NteHostRegion.Global => "NteGlobal", NteHostRegion.Taiwan => "NteTaiwan",
            _ => throw new ArgumentException("Unknown launch region.", nameof(region))
        }, "launch.json");

    internal static string GetPath(bool global) => Path.Combine(
        Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),
        "TouchUILaunch", "NTE", global ? "NteGlobal" : "NteCN", "launch.json");

    internal static (string GraphicsApi, bool OfficialLauncher) Read(string path)
    {
        if (!File.Exists(path)) return ("default", false);
        if (new FileInfo(path).Length is < 1 or > 4096) throw new InvalidDataException("Invalid launch preferences.");
        using JsonDocument document = JsonDocument.Parse(File.ReadAllBytes(path));
        JsonElement root = document.RootElement;
        string graphics = root.GetProperty("graphicsApi").GetString() ?? "";
        ValidateGraphics(graphics);
        bool official = root.TryGetProperty("officialLauncher", out JsonElement value) && value.GetBoolean();
        return (graphics, official);
    }

    internal static void Update(string path, string? graphicsApi = null, bool? officialLauncher = null)
    {
        if (graphicsApi != null) ValidateGraphics(graphicsApi);
        var current = Read(path);
        WriteAtomic(path, writer =>
        {
            writer.WriteString("graphicsApi", graphicsApi ?? current.GraphicsApi);
            writer.WriteBoolean("officialLauncher", officialLauncher ?? current.OfficialLauncher);
        });
    }

    internal static void ValidateGraphics(string value)
    {
        if (value is not ("default" or "dx11" or "dx12")) throw new ArgumentException("Unknown graphics API.", nameof(value));
    }

    internal static void WriteAtomic(string path, Action<Utf8JsonWriter> write)
    {
        path = Path.GetFullPath(path);
        Directory.CreateDirectory(Path.GetDirectoryName(path)!);
        string temporary = path + "." + Guid.NewGuid().ToString("N") + ".tmp";
        try
        {
            using (FileStream stream = new(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None,
                       4096, FileOptions.WriteThrough))
            {
                using Utf8JsonWriter writer = new(stream);
                writer.WriteStartObject();
                write(writer);
                writer.WriteEndObject();
                writer.Flush();
                stream.Flush(true);
            }
            File.Move(temporary, path, true);
        }
        finally { if (File.Exists(temporary)) File.Delete(temporary); }
    }
}
