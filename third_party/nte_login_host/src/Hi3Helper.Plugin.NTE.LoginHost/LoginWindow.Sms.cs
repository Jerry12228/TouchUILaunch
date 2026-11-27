using Avalonia.Threading;

namespace Nte.LoginHost;

internal sealed partial class LoginWindow
{
    private readonly DispatcherTimer smsTimer = new() { Interval = TimeSpan.FromSeconds(1) };
    private bool accountControlsEnabled;
    private DateTimeOffset previewNextSmsUtc;
    private int smsRemainingSeconds;

    private void ConfigureSmsCooldown()
    {
        smsTimer.Tick += (_, _) =>
        {
            UpdateSmsButton();
            if (preview) WritePreviewState();
        };
        sms.Click += async (_, _) => await SendSmsFromUiAsync();
    }

    private async Task SendSmsFromUiAsync()
    {
        if (!sms.IsEnabled) return;
        if (preview)
        {
            previewNextSmsUtc = DateTimeOffset.UtcNow.AddSeconds(60);
            status.Text = Localization.LoginText.Get("PreviewSmsCooldown");
            UpdateSmsButton(); WritePreviewState(); return;
        }
        await ExecuteAsync(async (api, token, generation) =>
        {
            status.Text = Localization.LoginText.Get("RequestingSmsCode");
            Task sending = api.SendSmsAsync((phone.Text ?? "").Trim(), token);
            UpdateSmsButton();
            try
            {
                await sending; RequireCurrent(generation, token);
                status.Text = Localization.LoginText.Get("SmsCodeSent");
            }
            finally { UpdateSmsButton(); }
        });
    }

    private void UpdateSmsButton()
    {
        TimeSpan remaining = preview ? previewNextSmsUtc - DateTimeOffset.UtcNow : client?.SmsCooldownRemaining ?? TimeSpan.Zero;
        smsRemainingSeconds = (int)Math.Clamp(Math.Ceiling(remaining.TotalSeconds), 0, 60);
        sms.Content = smsRemainingSeconds > 0 ? Localization.LoginText.Get("SmsResendCountdown", smsRemainingSeconds) : Localization.LoginText.Get("SendSmsCode");
        sms.IsEnabled = accountControlsEnabled && !busy && smsRemainingSeconds == 0;
        if (requested && !stopping && smsRemainingSeconds > 0) smsTimer.Start();
        else smsTimer.Stop();
    }
}
