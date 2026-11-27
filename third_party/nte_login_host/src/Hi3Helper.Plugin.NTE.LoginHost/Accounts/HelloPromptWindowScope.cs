using System.ComponentModel;
using System.Runtime.InteropServices;

namespace Nte.LoginHost.Accounts;

// Promote only our own CNG owner for the duration of a gesture. Win32 keeps
// owned dialogs above a topmost owner. Never locate or modify system dialogs.
internal sealed class HelloPromptWindowScope : IDisposable
{
    private readonly nint owner;
    private readonly bool wasTopmost;
    private bool disposed;

    internal HelloPromptWindowScope(nint owner, bool requestForeground = true)
    {
        if (owner == 0 || !WindowNative.IsWindow(owner))
            throw new InvalidOperationException("A live Hello owner is required.");
        WindowNative.GetWindowThreadProcessId(owner, out uint process);
        if (process != Environment.ProcessId)
            throw new InvalidOperationException("Hello owner must belong to the login host.");
        this.owner = owner;
        wasTopmost = (WindowNative.GetWindowLongPtr(owner, -20).ToInt64() & 8) != 0;
        uint flags = requestForeground ? 0x243u : 0x253u;
        if (!WindowNative.SetWindowPos(owner, -1, 0, 0, 0, 0, flags))
            throw new Win32Exception(Marshal.GetLastWin32Error());
        if (requestForeground) _ = SetForegroundWindow(owner);
    }

    public void Dispose()
    {
        if (disposed) return;
        disposed = true;
        if (!wasTopmost && WindowNative.IsWindow(owner) && !WindowNative.SetWindowPos(owner, -2, 0, 0, 0, 0, 0x213))
            throw new Win32Exception(Marshal.GetLastWin32Error());
    }

    [DllImport("user32.dll")]
    private static extern bool SetForegroundWindow(nint window);
}
