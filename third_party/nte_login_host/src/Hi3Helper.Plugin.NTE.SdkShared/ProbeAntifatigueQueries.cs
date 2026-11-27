// Fixed native-image semantics, shared by the live pipe loop and offline tests.
internal sealed class ProbeAntifatigueQueries(ProbeLoginAdmission admission, int clientId,
    Func<ProbePreLoginResponse, ProbePreLoginRequest, CancellationToken, Task> deliver, Action<object> observe)
{
    internal async Task HandleAsync(ReadOnlyMemory<byte> frame, CancellationToken token)
    {
        int type = ProbePreLoginRequest.ReadQueryType(frame.Span, clientId);
        if (type == 2)
        {
            ProbePreLoginRequest request = ProbePreLoginRequest.Parse(frame.Span, clientId, 2);
            // Native host 0x45FAD7 dispatches only types 0 and 1. Type 2 has
            // no request/response service. onRoleLoginComplete 0x586798 and
            // onHeartbeatComplete 0x58524B instead push actual service results
            // with context -1. Bridge 0x25C40 caches and immediately re-arms,
            // even after an error: replying here would cause an endless loop.
            // Those role/heartbeat producers remain a separate research task.
            observe(new { stage = "antifatigue-info-subscribed", operation = 40, queryType = 2, contextId = request.ContextId });
            return;
        }
        if (type != 1)
        {
            observe(new { stage = "sdk-query-unimplemented", operation = 40, queryType = type });
            return;
        }
        ProbePreLoginRequest preLogin = ProbePreLoginRequest.Parse(frame.Span, clientId);
        observe(new { stage = "antifatigue-query-received", contextId = preLogin.ContextId, queryType = 1 });
        await admission.ReplyAsync(async response =>
        {
            await deliver(response, preLogin, token).ConfigureAwait(false);
            observe(new { stage = "antifatigue-callback-delivered", contextId = preLogin.ContextId,
                queryType = 1, operation = 49, serverCode = response.Code });
        }, token).ConfigureAwait(false);
    }
}
