namespace Nte.LoginHost;

// Once the game has started, neither a UI nor SDK initialization failure may
// dispose its host before the game exits. The caller supplies the actual exit
// observation and cleanup, so this policy is testable without starting a game.
internal static class GameStartedLifetime
{
    internal static async Task<bool> RunAsync(Task gameExit, Func<Task> run, Func<Exception, Task> reportFailure,
        Func<bool> stopping, Func<Task> cleanup, Task? backgroundFailure = null)
    {
        bool succeeded = true;
        try
        {
            try
            {
                Task operation = run();
                await (backgroundFailure == null ? operation : await Task.WhenAny(operation, backgroundFailure));
            }
            catch (OperationCanceledException) when (stopping() || gameExit.IsCompleted) { }
            catch (Exception ex)
            {
                succeeded = false;
                if (!stopping() && !gameExit.IsCompleted)
                {
                    // If displaying the warning itself fails, still retain the
                    // SDK host until the game has exited.
                    try { await reportFailure(ex); }
                    catch (Exception) { }
                }
            }
        }
        finally
        {
            try { await gameExit; }
            finally { await cleanup(); }
        }
        return succeeded;
    }
}
