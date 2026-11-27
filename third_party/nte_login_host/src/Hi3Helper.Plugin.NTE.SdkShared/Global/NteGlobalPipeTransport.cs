using System.Buffers.Binary;
using System.IO.Pipes;
using System.Runtime.InteropServices;
using System.Security.Cryptography;
using System.Text.Json;

namespace Nte.Login;

// The Global bridge receives official replies on its registered duplex pipe.
// One lock covers a whole login sequence so metadata cannot split 1539/1538.
internal sealed class NteGlobalPipeTransport
{
    private readonly NamedPipeServerStream pipe;
    private readonly NteOverseasRegion region;
    private readonly SemaphoreSlim writes = new(1, 1);

    internal NteGlobalPipeTransport(NamedPipeServerStream pipe, NteOverseasRegion? region = null)
    {
        ArgumentNullException.ThrowIfNull(pipe);
        if (!pipe.IsConnected || !pipe.CanWrite) throw new IOException("Global SDK pipe is not connected.");
        this.pipe = pipe;
        this.region = region ?? NteOverseasRegion.Global;
    }

    internal Task SendLoginAsync(NteGlobalIdentity identity, CancellationToken token)
    {
        if (identity.Region != region) throw new InvalidDataException("The login result belongs to another publisher.");
        return SendPacketsAsync(NteGlobalLoginPacket.EncodeSequence(identity, Environment.ProcessId), token);
    }

    internal Task SendWindowsPlatformAsync(CancellationToken token) =>
        SendPacketsAsync([NteGlobalPacket.EncodeWindowsPlatform(Environment.ProcessId)], token);

    internal Task SendClientContextAsync(CancellationToken token) =>
        SendPacketsAsync(NteGlobalSdkContextPackets.Encode(Environment.ProcessId, region), token);

    internal Task SendQueryAsync(NteGlobalSdkQuery query, JsonElement response, CancellationToken token) =>
        SendPacketsAsync([NteGlobalPacket.EncodeQuery(query, response, Environment.ProcessId)], token);

    internal Task SendDeviceInfoAsync(NteGlobalDeviceInfo device, CancellationToken token) =>
        SendPacketsAsync([
            NteGlobalPacket.Encode(1551, Environment.ProcessId, json =>
            {
                json.WriteStartObject();
                json.WriteString("result", device.Json);
                json.WriteEndObject();
            })
        ], token);

    internal Task BindGameWindowAsync(nint window, int clientId, CancellationToken token)
    {
        if (window == 0 || clientId <= 0 || !IsWindow(window) ||
            GetWindowThreadProcessId(window, out uint owner) == 0 || owner != clientId)
            throw new InvalidOperationException("Global SDK window must belong to the owned client.");
        // Command 9 is an unencrypted HWND. The bridge uses it to dispatch
        // later pipe replies on the actual game's window thread.
        byte[] packet = new byte[16];
        BinaryPrimitives.WriteUInt32LittleEndian(packet, 0x6B647331);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(4), 9);
        BinaryPrimitives.WriteInt32LittleEndian(packet.AsSpan(8), packet.Length);
        BinaryPrimitives.WriteUInt32LittleEndian(packet.AsSpan(12), unchecked((uint)window));
        return SendPacketsAsync([packet], token);
    }

    internal Task SendLanguageAsync(string language, CancellationToken token)
    {
        string sdkLanguage = language.ToLowerInvariant() switch
        {
            "zh-cn" or "zh-hans" => "zh-Hans",
            "zh-tw" or "zh-hk" or "zh-mo" or "zh-hant" => "zh-Hant",
            _ => language.ToLowerInvariant()
        };
        return SendPacketsAsync([
            NteGlobalPacket.Encode(1546, Environment.ProcessId, json =>
            {
                json.WriteStartObject();
                json.WriteString("result", sdkLanguage);
                json.WriteEndObject();
            })
        ], token);
    }

    private async Task SendPacketsAsync(byte[][] packets, CancellationToken token)
    {
        bool locked = false;
        try
        {
            await writes.WaitAsync(token).ConfigureAwait(false);
            locked = true;
            foreach (byte[] packet in packets)
            {
                token.ThrowIfCancellationRequested();
                if (!pipe.IsConnected) throw new IOException("Global SDK pipe disconnected.");
                // One WriteAsync is one message. Never combine independent SDK
                // frames into a message, including the two login callbacks.
                await pipe.WriteAsync(packet, token).ConfigureAwait(false);
            }

            await pipe.FlushAsync(token).ConfigureAwait(false);
        }
        finally
        {
            if (locked) writes.Release();
            foreach (byte[] packet in packets) CryptographicOperations.ZeroMemory(packet);
        }
    }

    [DllImport("user32")]
    private static extern bool IsWindow(nint window);

    [DllImport("user32")]
    private static extern uint GetWindowThreadProcessId(nint window, out uint processId);
}
