namespace Nte.Login;

// Owns only this game's in-memory identity. A logout invalidates pending work
// before disposing the previous identity; no global account is saved to disk.
internal sealed class NteGlobalSession : IDisposable
{
    private NteGlobalIdentity? identity;
    private CancellationTokenSource generation = new();
    private bool disposed;
    internal CancellationToken Token => generation.Token;
    internal NteGlobalIdentity? Capture() => identity?.Clone();
    internal void Remember(NteGlobalIdentity value, CancellationToken sessionToken)
    {
        ObjectDisposedException.ThrowIf(disposed, this);
        sessionToken.ThrowIfCancellationRequested();
        if (sessionToken != generation.Token) throw new InvalidOperationException("Identity belongs to another login generation.");
        NteGlobalIdentity next = value.Clone();
        identity?.Dispose(); identity = next;
    }
    internal void Clear()
    {
        if (disposed) return;
        generation.Cancel(); generation.Dispose(); generation = new();
        identity?.Dispose(); identity = null;
    }
    public void Dispose()
    { if (disposed) return; disposed = true; generation.Cancel(); generation.Dispose(); identity?.Dispose(); identity = null; }
}
