using System.Collections.Concurrent;
using System.ComponentModel;
using System.Runtime.InteropServices;

// A window owned by this probe, not a window injected into the game. It records
// protocol headers only and never returns a successful SDK or login result.
internal sealed class ProbeWindow : IDisposable
{
    private readonly Thread thread;
    private readonly ManualResetEventSlim ready = new();
    private readonly WindowProcedure procedure;
    private Exception? startupError;
    private nint handle;
    public nint Handle => handle;
    public uint Notification { get; private set; }
    public string Title { get; } = "TouchUILaunch NTE SDK Probe " + Guid.NewGuid().ToString("N");
    public ConcurrentQueue<object> Messages { get; } = new();
    private readonly TaskCompletionSource loginRequested = new(TaskCreationOptions.RunContinuationsAsynchronously);
    private readonly ProbeLoginRequests requests = new();
    private readonly uint loginCommand;
    internal event Action<Nte.Login.NteGlobalSdkQuery>? GlobalQueryRequested;
    internal Task WaitForLoginRequestAsync(CancellationToken token) => loginRequested.Task.WaitAsync(token);
    internal Task WaitForNextLoginRequestAsync(CancellationToken token) => requests.WaitAsync(token);
    internal void CompleteLoginRequest() => requests.Complete();
    // Repeated requests may need to reveal the existing form while its
    // authentication callback is still pending.
    internal event Action? LoginRequested;
    // Command 14 is a geometry hint from the installed bridge's window
    // procedure. Its payload is deliberately not interpreted as an HWND.
    internal event Action? GameWindowChanged;
    // Fixed OneSdk_Logout posts notification 4; do not infer logout from logs.
    internal event Action? LogoutRequested;

    public ProbeWindow(uint loginCommand = 1)
    {
        if (loginCommand is not (1 or 100)) throw new ArgumentOutOfRangeException(nameof(loginCommand));
        this.loginCommand = loginCommand;
        procedure = Receive;
        thread = new Thread(Run) { IsBackground = true, Name = "NTE SDK probe window" };
        thread.SetApartmentState(ApartmentState.STA);
        thread.Start();
        if (!ready.Wait(TimeSpan.FromSeconds(10))) throw new TimeoutException("Probe window startup timed out.");
        if (startupError != null) throw new InvalidOperationException("Cannot create the probe window.", startupError);
    }

    private void Run()
    {
        string className = "TouchUILaunch.NTE.SdkProbe." + Guid.NewGuid().ToString("N");
        nint instance = GetModuleHandleW(null);
        try
        {
            Notification = RegisterWindowMessageW(className + ".Notify");
            WindowClass descriptor = new()
            {
                Size = (uint)Marshal.SizeOf<WindowClass>(), Procedure = procedure,
                Instance = instance, ClassName = className
            };
            if (Notification == 0 || RegisterClassExW(ref descriptor) == 0) throw new Win32Exception();
            handle = CreateWindowExW(0, className, Title, 0, 0, 0, 0, 0, 0, 0, instance, 0);
            if (handle == 0) throw new Win32Exception();
            ready.Set();
            int result;
            while ((result = GetMessageW(out Message message, 0, 0, 0)) > 0)
            {
                TranslateMessage(ref message);
                DispatchMessageW(ref message);
            }
            if (result < 0) Messages.Enqueue(new { kind = "message-loop-error", error = Marshal.GetLastWin32Error() });
        }
        catch (Exception ex) { startupError = ex; ready.Set(); }
        finally
        {
            if (handle != 0) { DestroyWindow(handle); handle = 0; }
            UnregisterClassW(className, instance);
        }
    }

    private nint Receive(nint window, uint message, nuint wParam, nint lParam)
    {
        try
        {
            if (message == 0x10) { DestroyWindow(window); return 0; }
            if (message == 2) { PostQuitMessage(0); return 0; }
            if (Messages.Count < 64 && message == 0x4A && lParam != 0)
            {
                CopyData data = Marshal.PtrToStructure<CopyData>(lParam);
                uint? operation = null;
                if (data.Length is >= 12 and <= 65536 && data.Data != 0 &&
                    unchecked((uint)Marshal.ReadInt32(data.Data)) == 0x6B647331)
                    operation = unchecked((uint)Marshal.ReadInt32(data.Data, 4));
                Messages.Enqueue(new { observedUtc = DateTimeOffset.UtcNow, kind = "copy-data", tag = (ulong)data.Tag, bytes = data.Length, operation });
                return 0; // An unimplemented API request is not acknowledged as success.
            }
            if (message == Notification)
            {
                if (loginCommand == 100 && Nte.Login.NteGlobalSdkQuery.TryCreate((uint)wParam, lParam, out var query))
                    GlobalQueryRequested?.Invoke(query);
                if (Messages.Count < 64)
                    Messages.Enqueue(new { observedUtc = DateTimeOffset.UtcNow, kind = "notification",
                        command = (uint)wParam, hasData = lParam != 0 });
                // The owned client's OneSdk_DoLogin installs its callback and
                // then sends command 1. Registration alone is too early.
                if (loginCommand == 100 && wParam is 101 or 102) LogoutRequested?.Invoke();
                if (wParam == loginCommand || loginCommand == 100 && wParam == 102)
                {
                    loginRequested.TrySetResult();
                    requests.Notify();
                    LoginRequested?.Invoke();
                }
                if (wParam == 14) GameWindowChanged?.Invoke();
                if (loginCommand == 1 && wParam == 4) LogoutRequested?.Invoke();
            }
        }
        catch { return 0; } // Exceptions must not cross the native window callback.
        return DefWindowProcW(window, message, wParam, lParam);
    }

    public void Dispose()
    {
        if (handle != 0) PostMessageW(handle, 0x10, 0, 0);
        if (!thread.Join(TimeSpan.FromSeconds(5))) throw new TimeoutException("Probe window did not stop.");
        ready.Dispose();
        GC.KeepAlive(procedure);
    }

    [UnmanagedFunctionPointer(CallingConvention.Winapi)]
    private delegate nint WindowProcedure(nint window, uint message, nuint wParam, nint lParam);
    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct WindowClass
    {
        public uint Size, Style;
        public WindowProcedure Procedure;
        public int ClassExtra, WindowExtra;
        public nint Instance, Icon, Cursor, Background;
        public string? MenuName;
        public string ClassName;
        public nint SmallIcon;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct Message
    {
        public nint Window;
        public uint Id;
        public nuint WParam;
        public nint LParam;
        public uint Time;
        public int X, Y;
        public uint Private;
    }
    [StructLayout(LayoutKind.Sequential)]
    private struct CopyData { public nuint Tag; public int Length; public nint Data; }

    [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)] private static extern nint GetModuleHandleW(string? name);
    [DllImport("user32", CharSet = CharSet.Unicode, SetLastError = true)] private static extern uint RegisterWindowMessageW(string name);
    [DllImport("user32", CharSet = CharSet.Unicode, SetLastError = true)] private static extern ushort RegisterClassExW(ref WindowClass descriptor);
    [DllImport("user32", CharSet = CharSet.Unicode, SetLastError = true)] private static extern nint CreateWindowExW(uint extendedStyle, string className, string title, uint style, int x, int y, int width, int height, nint parent, nint menu, nint instance, nint parameter);
    [DllImport("user32", CharSet = CharSet.Unicode, SetLastError = true)] private static extern bool UnregisterClassW(string className, nint instance);
    [DllImport("user32", SetLastError = true)] private static extern bool DestroyWindow(nint window);
    [DllImport("user32", CharSet = CharSet.Unicode)] private static extern nint DefWindowProcW(nint window, uint message, nuint wParam, nint lParam);
    [DllImport("user32")] private static extern void PostQuitMessage(int exitCode);
    [DllImport("user32", CharSet = CharSet.Unicode)] private static extern bool PostMessageW(nint window, uint message, nuint wParam, nint lParam);
    [DllImport("user32", CharSet = CharSet.Unicode, SetLastError = true)] private static extern int GetMessageW(out Message message, nint window, uint min, uint max);
    [DllImport("user32")] private static extern bool TranslateMessage(ref Message message);
    [DllImport("user32", CharSet = CharSet.Unicode)] private static extern nint DispatchMessageW(ref Message message);
}
