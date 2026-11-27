using System.Runtime.InteropServices;

namespace Nte.LoginHost;

internal static class WindowNative
{
    internal delegate bool EnumWindowsProcedure(nint window, nint parameter);
    internal delegate void WinEventProcedure(nint hook, uint eventType, nint window, int objectId, int childId, uint threadId, uint time);
    [StructLayout(LayoutKind.Sequential)] internal struct Rect { public int Left, Top, Right, Bottom; public int Width => Right - Left; public int Height => Bottom - Top; }
    [StructLayout(LayoutKind.Sequential)] internal struct Point { public int X, Y; }
    [DllImport("user32")] internal static extern bool EnumWindows(EnumWindowsProcedure callback, nint parameter);
    [DllImport("user32")] internal static extern bool EnumChildWindows(nint window, EnumWindowsProcedure callback, nint parameter);
    [DllImport("user32")] internal static extern nint GetParent(nint window);
    [DllImport("user32")] internal static extern nint GetWindow(nint window, uint command);
    [DllImport("user32")] internal static extern nint GetAncestor(nint window, uint flag);
    [DllImport("user32")] internal static extern nint GetWindowDpiAwarenessContext(nint window);
    [DllImport("user32")] internal static extern nint SetThreadDpiAwarenessContext(nint context);
    [DllImport("user32")] internal static extern bool AreDpiAwarenessContextsEqual(nint first, nint second);
    [DllImport("user32")] internal static extern nint SetWinEventHook(uint minimum, uint maximum, nint module, WinEventProcedure callback, uint processId, uint threadId, uint flags);
    [DllImport("user32")] internal static extern bool UnhookWinEvent(nint hook);
    [DllImport("user32")] internal static extern uint GetWindowThreadProcessId(nint window, out uint processId);
    [DllImport("user32")] internal static extern bool IsWindow(nint window);
    [DllImport("user32")] internal static extern bool IsWindowVisible(nint window);
    [DllImport("user32")] internal static extern bool IsIconic(nint window);
    [DllImport("user32")] internal static extern bool GetClientRect(nint window, out Rect rectangle);
    [DllImport("user32")] internal static extern bool ClientToScreen(nint window, ref Point point);
    [DllImport("user32")] internal static extern nint GetForegroundWindow();
    [DllImport("user32")] internal static extern bool GetCursorPos(out Point point);
    [DllImport("user32")] internal static extern nint WindowFromPoint(Point point);
    [DllImport("user32")] internal static extern uint GetDpiForWindow(nint window);
    [DllImport("user32", EntryPoint = "SetWindowLongPtrW", SetLastError = true)] internal static extern nint SetWindowLongPtr(nint window, int index, nint value);
    [DllImport("user32", EntryPoint = "GetWindowLongPtrW")] internal static extern nint GetWindowLongPtr(nint window, int index);
    [DllImport("user32")] internal static extern bool ShowWindow(nint window, int command);
    [DllImport("user32", SetLastError = true)] internal static extern bool SetWindowPos(nint window, nint after, int x, int y, int width, int height, uint flags);
    [DllImport("user32")] internal static extern nint GetDC(nint window);
    [DllImport("user32")] internal static extern int ReleaseDC(nint window, nint dc);

    internal static bool IsTopmost(nint window) => (GetWindowLongPtr(window, -20) & 8) != 0;

    internal static void SetOwner(nint window, nint owner)
    {
        if (GetWindow(window, 4) == owner) return;
        nint previous = SetWindowLongPtr(window, -8, owner);
        int error = Marshal.GetLastPInvokeError();
        if (previous == 0 && error != 0) throw new System.ComponentModel.Win32Exception(error);
    }

    internal static nint FindGameWindow(int processId)
    {
        nint best = 0; long area = 0;
        EnumWindows((window, _) =>
        {
            GetWindowThreadProcessId(window, out uint owner);
            if (owner == processId && IsWindowVisible(window) && GetClientRect(window, out Rect rect) &&
                rect.Width >= 320 && rect.Height >= 180 && (long)rect.Width * rect.Height > area)
            { best = window; area = (long)rect.Width * rect.Height; }
            return true;
        }, 0);
        return best;
    }

    internal static bool IsWindowAbove(nint window, nint reference)
    {
        if (window == 0 || reference == 0 || window == reference) return false;
        bool above = false;
        // EnumWindows follows the top-level Z order. A visible HWND and an
        // assigned owner alone do not prove that the game cannot cover it.
        EnumWindows((candidate, _) =>
        {
            if (candidate == window) { above = true; return false; }
            return candidate != reference;
        }, 0);
        return above;
    }

    internal static Rect ScreenClient(nint window)
    {
        if (!GetClientRect(window, out Rect rect)) return default;
        Point point = default;
        if (!ClientToScreen(window, ref point)) return default;
        return new() { Left = point.X, Top = point.Y, Right = point.X + rect.Width, Bottom = point.Y + rect.Height };
    }

}
