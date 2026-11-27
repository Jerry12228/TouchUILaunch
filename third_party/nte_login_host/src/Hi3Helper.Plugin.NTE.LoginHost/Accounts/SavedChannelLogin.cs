namespace Nte.LoginHost.Accounts;

internal static class SavedChannelLogin
{
    internal static async Task<ProbeIdentityEnvelope> RestoreAsync(SavedChannelCredential saved, ProbeAuthenticationClient api,
        Action<string> progress, CancellationToken token)
    {
        token.ThrowIfCancellationRequested();
        progress("正在初始化保存账号的登录环境…");
        await api.InitializeRestoredContextAsync(saved.DeviceId, saved.Platform, saved.ChannelId, token);
        token.ThrowIfCancellationRequested(); progress("正在校验保存的渠道凭据…");
        ProbeChannelValidationResult result = await api.ValidateChannelIdentityAsync(saved.Identity, token);
        token.ThrowIfCancellationRequested();
        // The isolated current-client reproduction returns HTTP 400 from the
        // optional validation request. That is not credential acceptance. In
        // this precise case, let the existing official identity exchange make
        // the authoritative decision. Service rejection, outage and required
        // interaction continue to stop this flow.
        if (result is not { Status: ProbeChannelValidationStatus.RequestRejected, Code: 400 }) result.RequireValidated();
        progress("正在重新取得本次游戏身份…");
        ProbeIdentityEnvelope game = await api.ExchangeGameIdentityAsync(saved.Identity, token);
        if (token.IsCancellationRequested) { game.Dispose(); token.ThrowIfCancellationRequested(); }
        return game;
    }
}
