// Local support policy: no verification UI, no guest/limited-session fallback.
// A successful backend identity exchange alone must never issue game login.
internal readonly record struct ProbeLoginDecision(bool Allowed, string Reason, string UserMessage)
{
    internal static ProbeLoginDecision Evaluate(int code, int? status, int? realUser, int? bannedType)
    {
        if (code != 0) return new(false, "prelogin-service-rejected", "服务端未通过登录前检查，本次登录已阻止。请稍后重试或使用官方启动器处理。");
        if (realUser == 0) return new(false, "real-name-required", "该账号尚未完成实名认证，本次登录已阻止。请先通过官方渠道完成实名认证后再登录。");
        if (status == 1 || bannedType > 0)
            return new(false, "real-name-restricted", "该账号的实名认证或防沉迷状态限制登录，本次登录已阻止。请通过官方渠道查看限制原因。");
        // The civicType/accountType branch at native 0x586005 writes a local
        // response cache through 0x5842F0; it is not a certification decision.
        // Real-name certification is a separate signal (0x7CF720).
        if (realUser != 1 || status != 0 || bannedType is < 0)
            return new(false, "prelogin-state-unavailable", "无法确认该账号的实名及防沉迷状态，本次登录已阻止。请稍后重试或使用官方启动器处理。");
        return new(true, "allowed", "");
    }

    internal void RequireAllowed(int serverCode)
    {
        if (!Allowed) throw new ProbeAuthenticationException(Reason, serverCode, UserMessage);
    }
}

// Serializes initial admission and refreshes, binds the result to the current
// game identity, and gates both the login callback and later pre-login reply.
internal sealed class ProbeLoginAdmission(ProbePreLoginClient client, Action<object> observe, TimeProvider? clock = null) : IDisposable
{
    private readonly SemaphoreSlim serial = new(1, 1);
    private readonly TimeProvider time = clock ?? TimeProvider.System;
    private ProbePreLoginResponse? response;
    private DateTimeOffset fetchedUtc;

    internal async Task ResetAsync(CancellationToken token)
    {
        await serial.WaitAsync(token).ConfigureAwait(false);
        try { response?.Dispose(); response = null; client.ClearIdentity(); }
        finally { serial.Release(); }
    }

    internal async Task AuthorizeAndDeliverAsync(ProbeIdentityEnvelope identity, Func<Task> deliver, CancellationToken token)
    {
        await serial.WaitAsync(token).ConfigureAwait(false);
        try
        {
            response?.Dispose(); response = null;
            client.SetIdentity(identity);
            await RefreshAsync(token).ConfigureAwait(false);
            Check();
            token.ThrowIfCancellationRequested();
            await deliver().ConfigureAwait(false);
        }
        finally { serial.Release(); }
    }

    internal async Task ReplyAsync(Func<ProbePreLoginResponse, Task> deliver, CancellationToken token)
    {
        await serial.WaitAsync(token).ConfigureAwait(false);
        try
        {
            if (response == null || time.GetUtcNow() - fetchedUtc >= TimeSpan.FromMinutes(1))
                await RefreshAsync(token).ConfigureAwait(false);
            Check();
            token.ThrowIfCancellationRequested();
            await deliver(response!).ConfigureAwait(false);
        }
        finally { serial.Release(); }
    }

    private async Task RefreshAsync(CancellationToken token)
    {
        using CancellationTokenSource deadline = CancellationTokenSource.CreateLinkedTokenSource(token);
        deadline.CancelAfter(TimeSpan.FromSeconds(25));
        ProbePreLoginResponse result = await client.QueryAsync(deadline.Token).ConfigureAwait(false);
        response?.Dispose(); response = result; fetchedUtc = time.GetUtcNow();
    }
    private void Check()
    {
        ProbeLoginDecision decision = response!.Decision;
        observe(new { stage = decision.Allowed ? "login-admission-allowed" : "login-admission-blocked", reason = decision.Reason,
            serverCode = response.Code });
        decision.RequireAllowed(response.Code);
    }
    public void Dispose() { response?.Dispose(); serial.Dispose(); }
}
