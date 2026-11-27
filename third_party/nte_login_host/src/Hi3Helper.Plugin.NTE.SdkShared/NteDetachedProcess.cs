using System.ComponentModel;
using System;
using System.Collections.Generic;
using System.IO;
using System.Runtime.InteropServices;
using System.Text;

namespace Nte.Login;

internal static class NteDetachedProcess
{
    // No inherited handles and no launcher job membership. If a restrictive
    // parent job refuses breakaway, fail before any game is created.
    internal static int Start(string executable, string option, string value)
    {
        if (option is not ("--game-root" or "--preview-pid")) throw new ArgumentException("Unsupported login host option.");
        return Start(executable, new[] { option, value });
    }
    internal static int Start(string executable, IReadOnlyList<string> arguments)
    {
        if (!Path.IsPathFullyQualified(executable) || !File.Exists(executable)) throw new FileNotFoundException("Login host executable is missing.");
        if (!IsProcessInJob(GetCurrentProcess(), 0, out bool inJob)) throw new Win32Exception();
        uint flags = 0x8 | 0x200 | (inJob ? 0x01000000u : 0);
        Startup startup = new() { Size = (uint)Marshal.SizeOf<Startup>() };
        StringBuilder command = new(Quote(executable));
        foreach (string argument in arguments) command.Append(' ').Append(Quote(argument));
        if (command.Length >= 32767) throw new ArgumentException("Login host command exceeds the Windows argument capacity.");
        if (!CreateProcessW(executable, command, 0, 0, false, flags, 0, Path.GetDirectoryName(executable), ref startup, out ProcessInfo process))
            throw new Win32Exception(Marshal.GetLastWin32Error(), "Independent login host could not leave the launcher's job.");
        try { return checked((int)process.Id); }
        finally { CloseHandle(process.Thread); CloseHandle(process.Process); }
    }
    private static string Quote(string value)
    {
        if (value.IndexOf('\0') >= 0) throw new ArgumentException("Invalid process argument.");
        StringBuilder result = new("\""); int slashes = 0;
        foreach (char c in value)
        {
            if (c == '\\') { slashes++; continue; }
            result.Append('\\', c == '"' ? slashes * 2 + 1 : slashes); result.Append(c); slashes = 0;
        }
        return result.Append('\\', slashes * 2).Append('"').ToString();
    }
    [StructLayout(LayoutKind.Sequential)] private struct Startup
    {
        public uint Size; public nint Reserved, Desktop, Title; public uint X, Y, Width, Height, XCount, YCount, Fill, Flags;
        public ushort Show, ReservedSize; public nint ReservedBytes, Input, Output, Error;
    }
    [StructLayout(LayoutKind.Sequential)] private struct ProcessInfo { public nint Process, Thread; public uint Id, ThreadId; }
    [DllImport("kernel32")] private static extern nint GetCurrentProcess();
    [DllImport("kernel32", SetLastError = true)] private static extern bool IsProcessInJob(nint process, nint job, out bool result);
    [DllImport("kernel32", CharSet = CharSet.Unicode, SetLastError = true)] private static extern bool CreateProcessW(string app, StringBuilder command, nint processAttributes, nint threadAttributes, bool inheritHandles, uint flags, nint environment, string? directory, ref Startup startup, out ProcessInfo information);
    [DllImport("kernel32")] private static extern bool CloseHandle(nint handle);
}
