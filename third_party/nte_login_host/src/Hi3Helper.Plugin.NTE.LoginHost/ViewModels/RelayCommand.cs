using System;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;

namespace Nte.LoginHost.ViewModels;

internal sealed class RelayCommand(Action action, Func<bool>? canExecute = null) : ICommand
{
    public event EventHandler? CanExecuteChanged;
    public bool CanExecute(object? parameter) => canExecute?.Invoke() ?? true;
    public void Execute(object? parameter) { if (CanExecute(parameter)) action(); }
    internal void Refresh() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
}

internal sealed class AsyncRelayCommand(Func<CancellationToken, Task> action, Func<bool>? canExecute = null) : ICommand, IDisposable
{
    private CancellationTokenSource? cancellation;
    private int running;
    public event EventHandler? CanExecuteChanged;
    internal bool IsRunning => Volatile.Read(ref running) != 0;
    public bool CanExecute(object? parameter) => !IsRunning && (canExecute?.Invoke() ?? true);
    public async void Execute(object? parameter) => await ExecuteAsync();
    internal async Task ExecuteAsync()
    {
        if (!CanExecute(null) || Interlocked.CompareExchange(ref running, 1, 0) != 0) return;
        cancellation = new(); Refresh();
        try { await action(cancellation.Token); }
        finally { cancellation.Dispose(); cancellation = null; Volatile.Write(ref running, 0); Refresh(); }
    }
    internal void Cancel() => cancellation?.Cancel();
    internal void Refresh() => CanExecuteChanged?.Invoke(this, EventArgs.Empty);
    public void Dispose() => Cancel();
}
