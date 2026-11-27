using Nte.Login;
using System.Diagnostics;
using System.IO.Pipes;
using System.Text;
using System.Text.Json;
using System.Runtime.InteropServices;
using Microsoft.Win32;

namespace Nte.LoginHost;

// The vendor's public pages return through wmgooglelogin. While an explicit
// login is pending, route that scheme to this host, then restore its previous
// command. This does not start GlobalLoginHelper or any official launcher.
internal sealed class GlobalBrowserLogin : IDisposable
{
    private const string CommandKey = @"Software\Classes\wmgooglelogin\shell\open\command";
    private readonly NteOverseasRegion region;
    private readonly string command;
    private readonly object? previous;
    private readonly RegistryValueKind previousKind;
    private readonly bool hadPrevious;
    private bool disposed;
    internal string Target { get; } = "collapse-nte-" + Guid.NewGuid().ToString("N");
    private readonly NamedPipeServerStream pipe;

    internal GlobalBrowserLogin(string? callbackExecutable = null, NteOverseasRegion? region = null)
    {
        this.region = region ?? NteOverseasRegion.Global;
        string executable = callbackExecutable ?? Environment.ProcessPath ?? throw new InvalidOperationException("Login apphost unavailable.");
        if (!Path.GetFileName(executable).Equals("Nte.LoginHost.exe", StringComparison.OrdinalIgnoreCase))
            throw new InvalidOperationException("Browser callbacks require the installed login apphost.");
        command = "\"" + executable + "\" --global-web-callback \"%1\"";
        pipe = BrowserCallbackPipe.Create("TouchUILaunch.NTE.Global.Web." + Target);
        try
        {
            using RegistryKey scheme = Registry.CurrentUser.CreateSubKey(@"Software\Classes\wmgooglelogin", true);
            // Keep the official scheme metadata and previous registration.
            scheme.SetValue("URL Protocol", "", RegistryValueKind.String);
            using RegistryKey key = Registry.CurrentUser.CreateSubKey(CommandKey, true);
            hadPrevious = key.GetValueNames().Contains(""); previous = key.GetValue("", null, RegistryValueOptions.DoNotExpandEnvironmentNames);
            previousKind = hadPrevious ? key.GetValueKind("") : RegistryValueKind.String;
            key.SetValue("", command, RegistryValueKind.String);
            SHChangeNotify(0x08000000, 0x1000, 0, 0); // SHCNE_ASSOCCHANGED | SHCNF_FLUSH before opening the browser.
        }
        catch { pipe.Dispose(); throw; }
    }

    internal async Task<JsonDocument> WaitAsync(Uri loginUri, CancellationToken token)
    {
        if (!region.IsBrowserOrigin(loginUri)) throw new InvalidDataException("Unexpected browser origin.");
        Trace("waiting", 0);
        Task connection = pipe.WaitForConnectionAsync(token);
        Process.Start(new ProcessStartInfo(loginUri.AbsoluteUri) { UseShellExecute = true });
        return await ReceiveAsync(connection, token);
    }
    internal Task<JsonDocument> ReceiveAsync(CancellationToken token) => ReceiveAsync(pipe.WaitForConnectionAsync(token), token);
    private async Task<JsonDocument> ReceiveAsync(Task connection, CancellationToken token)
    {
        await connection;
        Trace("connected", 0);
        byte[] size = new byte[4]; await pipe.ReadExactlyAsync(size, token);
        int length = System.Buffers.Binary.BinaryPrimitives.ReadInt32LittleEndian(size);
        if (length is < 2 or > 60 * 1024) throw new InvalidDataException("Browser callback size bound.");
        byte[] data = new byte[length];
        try
        {
            await pipe.ReadExactlyAsync(data, token);
            Trace("received", length);
            Utf8JsonReader reader = new(data);
            return JsonDocument.ParseValue(ref reader);
        }
        finally { System.Security.Cryptography.CryptographicOperations.ZeroMemory(data); }
    }

    private void Trace(string stage, int bytes)
    {
        try
        {
            string directory = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "TouchUILaunch", "NTE", "Diagnostics");
            Directory.CreateDirectory(directory);
            string path = Path.Combine(directory, "browser-callback.log");
            if (File.Exists(path) && new FileInfo(path).Length > 64 * 1024) File.Move(path, path + ".previous", true);
            // Never record the callback target, URL, provider fields or account.
            File.AppendAllText(path, DateTimeOffset.UtcNow.ToString("O") + " " + region.Name + " " + stage + " bytes=" + bytes + Environment.NewLine);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException) { }
    }

    internal static async Task<bool> ForwardAsync(string uriText)
    {
        if (uriText.Length > 128 * 1024 || !Uri.TryCreate(uriText, UriKind.Absolute, out Uri? uri) || uri.Scheme != "wmgooglelogin" ||
            !System.Text.RegularExpressions.Regex.IsMatch(uri.Host, @"\Acollapse-nte-[a-f0-9]{32}\z") ||
            uri.UserInfo.Length != 0 || !uri.IsDefaultPort || uri.AbsolutePath is not ("" or "/") || uri.Fragment.Length != 0)
            return false;
        var parameters = uri.Query.TrimStart('?').Split('&');
        if (parameters.Length != 1 || !parameters[0].StartsWith("callback=", StringComparison.Ordinal)) return false;
        string json = Uri.UnescapeDataString(parameters[0][9..]);
        byte[] data = Encoding.UTF8.GetBytes(json);
        try
        {
            if (data.Length is < 2 or > 60 * 1024) return false;
            using JsonDocument document = JsonDocument.Parse(data);
            if (document.RootElement.ValueKind != JsonValueKind.Object) return false;
            using CancellationTokenSource timeout = new(TimeSpan.FromSeconds(5));
            using NamedPipeClientStream client = new(".", "TouchUILaunch.NTE.Global.Web." + uri.Host, PipeDirection.Out, PipeOptions.Asynchronous);
            await client.ConnectAsync(timeout.Token);
            if (!BrowserCallbackPipe.IsOwnedByCurrentUser(client)) return false;
            byte[] size = new byte[4]; System.Buffers.Binary.BinaryPrimitives.WriteInt32LittleEndian(size, data.Length);
            await client.WriteAsync(size, timeout.Token); await client.WriteAsync(data, timeout.Token); await client.FlushAsync(timeout.Token);
            return true;
        }
        catch (Exception ex) when (ex is IOException or JsonException or OperationCanceledException or UnauthorizedAccessException) { return false; }
        finally { System.Security.Cryptography.CryptographicOperations.ZeroMemory(data); }
    }

    public void Dispose()
    {
        if (disposed) return; disposed = true; pipe.Dispose();
        using RegistryKey? key = Registry.CurrentUser.OpenSubKey(CommandKey, true);
        // Do not overwrite a registration changed by the user or another app.
        if (key?.GetValue("") as string != command) return;
        if (hadPrevious && previous != null) key.SetValue("", previous, previousKind);
        else key.DeleteValue("", false);
        SHChangeNotify(0x08000000, 0x1000, 0, 0);
    }
    [DllImport("shell32.dll")] private static extern void SHChangeNotify(uint eventId, uint flags, nint item1, nint item2);
}
