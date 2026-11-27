using System.Text.Json;
using System.Threading.Channels;
using Nte.Login;
using Nte.LoginHost.Localization;

namespace Nte.LoginHost;

// Metadata queries have their own bounded worker. The native window callback
// only enqueues a context ID; it never waits for HTTP or opens a login panel.
internal sealed class GlobalSdkQueries(NteGlobalAuthentication api, LoginWindow window)
{
    private readonly Channel<NteGlobalSdkQuery> requests = Channel.CreateBounded<NteGlobalSdkQuery>(
        new BoundedChannelOptions(32) { SingleReader = true, SingleWriter = true });

    internal void Enqueue(NteGlobalSdkQuery request) => requests.Writer.TryWrite(request);

    internal async Task RunAsync(NteGlobalPipeTransport callback, CancellationToken token)
    {
        try
        {
            await foreach (NteGlobalSdkQuery request in requests.Reader.ReadAllAsync(token).ConfigureAwait(false))
            {
                try
                {
                    using JsonDocument response = await api.QuerySdkInfoAsync(request, token).ConfigureAwait(false);
                    await callback.SendQueryAsync(request, response.RootElement, token).ConfigureAwait(false);
                }
                catch (OperationCanceledException) when (token.IsCancellationRequested)
                {
                    throw;
                }
                catch (NteGlobalAuthenticationException error)
                {
                    window.ReportServiceFailure(LoginText.Get(error.Key, error.Arguments));
                }
                catch (Exception error) when (error is HttpRequestException or IOException or InvalidOperationException
                                                  or JsonException or System.ComponentModel.Win32Exception)
                {
                    window.ReportServiceFailure(LoginText.Get("ServiceUnavailable"));
                }
            }
        }
        catch (OperationCanceledException) when (token.IsCancellationRequested)
        {
        }
    }
}
