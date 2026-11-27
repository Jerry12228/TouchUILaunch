using Microsoft.Win32.SafeHandles;
using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Runtime.Versioning;
using System.Text;
using System.Threading;
using System.Threading.Tasks;

namespace Hi3Helper.Plugin.NTE.Management.Game;

internal static class NteProcessImage
{
    private const uint QueryLimitedInformation = 0x1000;

    // MainModule can be unavailable when the game is elevated or protected,
    // even when its image can be queried with limited process rights.
    [SupportedOSPlatform("windows")]
    internal static bool Matches(int processId, string expectedImage)
    {
        if (processId <= 0 || string.IsNullOrEmpty(expectedImage)) return false;
        using SafeProcessHandle process = OpenProcess(QueryLimitedInformation, false, processId);
        if (process.IsInvalid) return false;
        return Matches(process, expectedImage);
    }

    [SupportedOSPlatform("windows")]
    private static bool Matches(SafeProcessHandle process, string expectedImage)
    {
        StringBuilder image = new(32768);
        int capacity = image.Capacity;
        return QueryFullProcessImageNameW(process, 0, image, ref capacity) &&
            string.Equals(image.ToString(), expectedImage, StringComparison.OrdinalIgnoreCase);
    }

    [SupportedOSPlatform("windows")]
    internal static async Task WaitForExitWithLimitedAccessAsync(int processId, string expectedImage, CancellationToken token)
    {
        string processName = Path.GetFileNameWithoutExtension(expectedImage);
        using SafeProcessHandle process = OpenProcess(QueryLimitedInformation, false, processId);
        if (process.IsInvalid)
        {
            int error = Marshal.GetLastWin32Error();
            if (!IsListed(null, processId)) return; // It exited between discovery and OpenProcess.
            throw new Win32Exception(error);
        }
        // Verify the opened process, not just the PID previously found by the
        // launcher. Holding this handle also prevents PID reuse during the wait.
        if (!Matches(process, expectedImage))
        {
            if (!IsListed(null, processId)) return;
            throw new InvalidOperationException("Game process image changed before waiting.");
        }
        while (true)
        {
            token.ThrowIfCancellationRequested();
            if (!GetExitCodeProcess(process, out uint exitCode)) throw new Win32Exception(Marshal.GetLastWin32Error());
            if (exitCode != 259 || !IsListed(processName, processId)) return;
            // The process deliberately may not grant SYNCHRONIZE. Limited
            // queries at one-second intervals are a bounded fallback, not a
            // UI-thread wait, and support cancellation.
            await Task.Delay(TimeSpan.FromSeconds(1), token).ConfigureAwait(false);
        }
    }

    private static bool IsListed(string? processName, int processId)
    {
        Process[] processes = processName == null ? Process.GetProcesses() : Process.GetProcessesByName(processName);
        try
        {
            foreach (Process candidate in processes)
                if (candidate.Id == processId) return true;
            return false;
        }
        finally { foreach (Process candidate in processes) candidate.Dispose(); }
    }

    [DllImport("kernel32.dll", SetLastError = true)]
    private static extern SafeProcessHandle OpenProcess(uint access, [MarshalAs(UnmanagedType.Bool)] bool inheritHandle, int processId);
    [DllImport("kernel32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool QueryFullProcessImageNameW(SafeProcessHandle process, uint flags, StringBuilder image, ref int size);
    [DllImport("kernel32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetExitCodeProcess(SafeProcessHandle process, out uint exitCode);
}
