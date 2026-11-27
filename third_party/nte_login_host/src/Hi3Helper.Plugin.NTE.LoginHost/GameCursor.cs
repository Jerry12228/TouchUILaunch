using System.Runtime.InteropServices;
using Avalonia.Threading;

namespace Nte.LoginHost;

// Borrow the cursor Windows is displaying for the game. Its handle retains
// the original size, hotspot, and system cursor behavior.
internal sealed class GameCursor : IDisposable
{
    private const int ClassCursor = -12; // GCLP_HCURSOR
    private nint cursor;
    private readonly DispatcherTimer refresh = new() { Interval = TimeSpan.FromMilliseconds(40) };
    private nint coverWindow, originalCursor;
    internal bool HasCursor { get; private set; }
    internal long ObservedHandle => (long)cursor;
    internal int Synchronizations { get; private set; }

    internal GameCursor()
    {
        refresh.Tick += (_, _) =>
        {
            // A control can change Avalonia's native cursor after the Win32
            // message. Check again only while the pointer is over our cover.
            if (!IsPointerOver(coverWindow)) refresh.Stop();
            else if (!Apply()) Reset();
        };
    }

    internal void ObserveGame(nint game)
    {
        CursorInfo info = new() { Size = (uint)Marshal.SizeOf<CursorInfo>() };
        // Touch or pen can temporarily suppress a hardware cursor. That
        // should not overwrite the last cursor the game selected.
        if (!GetCursorInfo(ref info) || (info.Flags & 2) != 0 || !MatchesWindowAt(info.Position, game))
            return;
        nint selected = (info.Flags & 1) != 0 ? info.Cursor : 0;
        if (!HasCursor || cursor != selected)
        {
            cursor = selected;
            HasCursor = true;
            Synchronizations++;
        }
    }

    internal bool Synchronize(nint cover)
    {
        if (!HasCursor || !IsPointerOver(cover)) return false;
        if (coverWindow != cover)
        {
            coverWindow = cover;
            originalCursor = GetClassLongPtrW(cover, ClassCursor);
        }
        if (!Apply())
        {
            Reset();
            return false;
        }
        if (!refresh.IsEnabled) refresh.Start();
        return true;
    }

    private static bool MatchesWindowAt(WindowNative.Point point, nint window)
        => window != 0 && WindowNative.GetAncestor(WindowNative.WindowFromPoint(point), 2) == window;

    private static bool IsPointerOver(nint window)
        => WindowNative.GetCursorPos(out var point) && MatchesWindowAt(point, window);

    private bool Apply()
    {
        // Use the game's cursor as the window class default as well, so
        // DefWindowProc cannot restore a different size on the next move.
        if (GetClassLongPtrW(coverWindow, ClassCursor) != cursor)
            SetClassLongPtrW(coverWindow, ClassCursor, cursor);
        SetCursor(cursor);
        // The source may have destroyed a custom cursor after we observed it.
        return GetCursor() == cursor;
    }

    internal void Reset()
    {
        refresh.Stop();
        if (coverWindow != 0 && WindowNative.IsWindow(coverWindow) &&
            GetClassLongPtrW(coverWindow, ClassCursor) == cursor)
            SetClassLongPtrW(coverWindow, ClassCursor, originalCursor);
        cursor = coverWindow = originalCursor = 0;
        HasCursor = false;
    }

    public void Dispose() => Reset();

    [StructLayout(LayoutKind.Sequential)]
    private struct CursorInfo
    {
        internal uint Size, Flags;
        internal nint Cursor;
        internal WindowNative.Point Position;
    }

    [DllImport("user32")]
    private static extern bool GetCursorInfo(ref CursorInfo info);

    [DllImport("user32")]
    private static extern nint GetCursor();

    [DllImport("user32")]
    private static extern nint SetCursor(nint cursor);

    [DllImport("user32")]
    private static extern nint GetClassLongPtrW(nint window, int index);

    [DllImport("user32")]
    private static extern nint SetClassLongPtrW(nint window, int index, nint value);
}
