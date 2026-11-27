using System.IO;
using System.Text.Json;

namespace Nte.LoginHost;

internal sealed partial class LoginWindow
{
    private async Task<bool> WaitForPreviewLoginAsync(ProbeWindow bridge, Task gameExit)
    {
        bridge.LoginRequested += NotifySdkLoginRequested;
        using CancellationTokenSource waiting = new();
        Task login = bridge.WaitForNextLoginRequestAsync(waiting.Token);
        File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "preview-sdk.json"),
            JsonSerializer.Serialize(new Dictionary<string, object?>
            {
                ["hostPid"] = Environment.ProcessId,
                ["window"] = (long)bridge.Handle,
                ["message"] = (long)bridge.Notification
            }, NteJsonContext.Default.DictionaryStringObject));
        if (await Task.WhenAny(login, gameExit) == login) return true;
        waiting.Cancel();
        return false;
    }

    internal void PreviewFirstUseNotice()
    {
        if (!preview) throw new InvalidOperationException("Synthetic preview required.");
        noticeCompletion = new(TaskCreationOptions.RunContinuationsAsynchronously);
        noticeGlobal = globalView != null;
        ShowLoginNotice();
    }
    private string? lastPreviewAction;
    private bool previewActionRunning, previewFollowForeground;

    // Isolated, non-authenticating fixture commands. This path is never used
    // by a real login session and only touches the in-memory preview accounts.
    private async Task ProcessPreviewActionAsync()
    {
        if (!preview || previewActionRunning) return;
        string commandPath = Path.Combine(AppContext.BaseDirectory, "preview-command.json");
        if (!File.Exists(commandPath) || new FileInfo(commandPath).Length > 2048) return;
        previewActionRunning = true;
        string? id = null;
        Exception? failure = null;
        try
        {
            using JsonDocument document = JsonDocument.Parse(File.ReadAllBytes(commandPath));
            JsonElement command = document.RootElement;
            id = command.GetProperty("id").GetString()!;
            if (id == lastPreviewAction) return;
            lastPreviewAction = id;
            switch (command.GetProperty("action").GetString())
            {
                // Simulate a deferred first presentation without moving or
                // activating the fixture. Discovery must recover it unaided.
                case "defer-presentation": HidePresented(); break;
                case "defer-native-presentation": WindowNative.ShowWindow(handle, 0); break;
                case "follow-foreground": previewFollowForeground = true; Track(); break;
                case "defer-z-order":
                    if (!WindowNative.SetWindowPos(handle, 1, 0, 0, 0, 0, 0x213))
                        throw new System.ComponentModel.Win32Exception();
                    break;
                case "send-sms": await SendSmsFromUiAsync(); break;
                case "accounts": await SwitchModeAsync(true); break;
                case "sms": await SwitchModeAsync(false); break;
                case "remove":
                    Guid account = command.GetProperty("account").GetGuid();
                    await RemoveAccountAsync(previewAccounts.Single(a => a.Id == account)); break;
                default: throw new InvalidDataException("Unknown preview action.");
            }
            WritePreviewState();
        }
        catch (Exception error) { failure = error; }
        finally { previewActionRunning = false; }
        File.WriteAllText(Path.Combine(AppContext.BaseDirectory, "preview-action-result.json"), JsonSerializer.Serialize(new Dictionary<string, object?>
        {
            ["id"] = id,
            ["passed"] = failure == null,
            ["error"] = failure?.ToString()
        }, NteJsonContext.Default.DictionaryStringObject));
    }
}
