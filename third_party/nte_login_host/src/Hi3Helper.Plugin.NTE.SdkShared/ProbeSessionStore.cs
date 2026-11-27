using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

// Explicit user-authorized cache. Only DPAPI ciphertext is written to disk;
// plaintext and native allocations are cleared after each save/load operation.
internal static class ProbeSessionStore
{
    private static readonly byte[] Entropy = "TouchUILaunch.NTE.IndependentLogin.v1"u8.ToArray();
    internal static string DefaultPath => Path.GetFullPath("artifacts/private-login/session.dpapi");

    internal static void Save(string path, JsonElement configuration, string version, string device,
        ReadOnlySpan<byte> applicationKey, ReadOnlySpan<byte> uid, ReadOnlySpan<byte> token)
    {
        if (applicationKey.Length != 16 || uid.Length is < 1 or > 63 || token.Length is < 1 or > 127)
            throw new InvalidDataException("Invalid session field capacity.");
        using MemoryStream output = new();
        byte[] plain = [], encrypted = [];
        try
        {
            using (Utf8JsonWriter writer = new(output))
            {
                writer.WriteStartObject(); writer.WriteNumber("format", 1);
                writer.WriteString("savedUtc", DateTimeOffset.UtcNow);
                writer.WriteNumber("appId", 1289); writer.WriteNumber("channelId", 9); writer.WriteNumber("platform", 4);
                writer.WriteString("installedVersion", version); writer.WriteString("deviceId", device);
                writer.WriteBase64String("applicationKeyTail", applicationKey);
                writer.WriteBase64String("channelUid", uid); writer.WriteBase64String("channelToken", token);
                writer.WritePropertyName("configuration"); configuration.WriteTo(writer);
                writer.WriteEndObject(); writer.Flush();
            }
            plain = output.ToArray();
            if (plain.Length > 16384) throw new InvalidDataException("Session size bound.");
            encrypted = Transform(plain, protect: true);
            string fullPath = Path.GetFullPath(path);
            Directory.CreateDirectory(Path.GetDirectoryName(fullPath)!);
            string temporary = fullPath + "." + Guid.NewGuid().ToString("N") + ".tmp";
            try
            {
                using (FileStream file = new(temporary, FileMode.CreateNew, FileAccess.Write, FileShare.None, 4096, FileOptions.WriteThrough))
                { file.Write(encrypted); file.Flush(flushToDisk: true); }
                File.Move(temporary, fullPath, overwrite: true);
            }
            finally { if (File.Exists(temporary)) File.Delete(temporary); }
        }
        finally
        {
            CryptographicOperations.ZeroMemory(plain); CryptographicOperations.ZeroMemory(encrypted);
            if (output.TryGetBuffer(out ArraySegment<byte> buffer)) CryptographicOperations.ZeroMemory(buffer.AsSpan());
        }
    }

    internal static T Read<T>(string path, Func<JsonElement, T> read)
    {
        if (new FileInfo(path).Length is < 1 or > 65536) throw new InvalidDataException("Invalid encrypted session size.");
        byte[] encrypted = File.ReadAllBytes(path), plain = [];
        try
        {
            plain = Transform(encrypted, protect: false);
            if (plain.Length > 16384) throw new InvalidDataException("Session size bound.");
            using JsonDocument document = JsonDocument.Parse(plain);
            JsonElement root = document.RootElement;
            DateTimeOffset saved = root.GetProperty("savedUtc").GetDateTimeOffset();
            if (root.GetProperty("format").GetInt32() != 1 || root.GetProperty("appId").GetInt32() != 1289 ||
                root.GetProperty("channelId").GetInt32() != 9 || root.GetProperty("platform").GetInt32() != 4 ||
                saved > DateTimeOffset.UtcNow.AddMinutes(1) || saved < DateTimeOffset.UtcNow.AddHours(-24))
                throw new InvalidDataException("Session metadata is incompatible or its local 24-hour lease has expired.");
            return read(root); // Caller must copy any values it needs before return.
        }
        finally { CryptographicOperations.ZeroMemory(encrypted); CryptographicOperations.ZeroMemory(plain); }
    }

    private static byte[] Transform(byte[] input, bool protect)
    {
        DataBlob source = Allocate(input), entropy = Allocate(Entropy), destination = default;
        nint description = 0;
        try
        {
            bool success = protect
                ? CryptProtectData(ref source, "TouchUILaunch NTE local test session", ref entropy, 0, 0, 1, out destination)
                : CryptUnprotectData(ref source, out description, ref entropy, 0, 0, 1, out destination);
            if (!success) throw new Win32Exception(Marshal.GetLastWin32Error(), "Current-user DPAPI operation failed.");
            if (destination.Length is < 1 or > 65536) throw new InvalidDataException("DPAPI output size bound.");
            byte[] result = new byte[destination.Length]; Marshal.Copy(destination.Data, result, 0, result.Length); return result;
        }
        finally
        {
            Clear(source, local: false); Clear(entropy, local: false); Clear(destination, local: true);
            if (description != 0) LocalFree(description);
        }
    }
    private static DataBlob Allocate(byte[] bytes)
    {
        nint pointer = Marshal.AllocHGlobal(bytes.Length); Marshal.Copy(bytes, 0, pointer, bytes.Length);
        return new() { Length = bytes.Length, Data = pointer };
    }
    private static void Clear(DataBlob blob, bool local)
    {
        if (blob.Data == 0) return;
        if (blob.Length is > 0 and <= 65536) Marshal.Copy(new byte[blob.Length], 0, blob.Data, blob.Length);
        if (local) LocalFree(blob.Data); else Marshal.FreeHGlobal(blob.Data);
    }
    [StructLayout(LayoutKind.Sequential)] private struct DataBlob { public int Length; public nint Data; }
    [DllImport("crypt32", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
    private static extern bool CryptProtectData(ref DataBlob input, string description, ref DataBlob entropy, nint reserved, nint prompt, uint flags, out DataBlob output);
    [DllImport("crypt32", CharSet = CharSet.Unicode, ExactSpelling = true, SetLastError = true)]
    private static extern bool CryptUnprotectData(ref DataBlob input, out nint description, ref DataBlob entropy, nint reserved, nint prompt, uint flags, out DataBlob output);
    [DllImport("kernel32")] private static extern nint LocalFree(nint pointer);
}
