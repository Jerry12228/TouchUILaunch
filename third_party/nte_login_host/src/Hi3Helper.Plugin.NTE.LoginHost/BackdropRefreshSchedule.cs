namespace Nte.LoginHost;

// Absolute deadlines from the first capture. Coalesce overdue slots after a
// background/minimized interval so returning never causes a capture burst.
internal sealed class BackdropRefreshSchedule
{
    private static readonly int[] Seconds = [1, 2, 5, 15, 60];
    private TimeSpan? started;
    private int next;

    internal void Start(TimeSpan now) { started = now; next = 0; }
    internal void Stop() { started = null; next = 0; }
    internal TimeSpan? DelayUntilNext(TimeSpan now) => started is TimeSpan origin && next < Seconds.Length
        ? TimeSpan.FromSeconds(Seconds[next]) - (now - origin) : null;
    internal bool TryConsumeDue(TimeSpan now)
    {
        if (started is not TimeSpan origin || next == Seconds.Length || now - origin < TimeSpan.FromSeconds(Seconds[next])) return false;
        do { next++; } while (next < Seconds.Length && now - origin >= TimeSpan.FromSeconds(Seconds[next]));
        return true;
    }
}
