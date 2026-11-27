// A login callback is one-shot in the client. Coalesce duplicate notifications
// until it is consumed, but allow the next legitimate login after completion.
internal sealed class ProbeLoginRequests
{
    private readonly SemaphoreSlim signal = new(0, 1);
    private int pending;
    internal void Notify()
    {
        if (Interlocked.CompareExchange(ref pending, 1, 0) == 0) signal.Release();
    }
    internal Task WaitAsync(CancellationToken token) => signal.WaitAsync(token);
    internal void Complete() => Interlocked.Exchange(ref pending, 0);
}
