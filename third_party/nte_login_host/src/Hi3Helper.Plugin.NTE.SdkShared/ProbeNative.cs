using System.Runtime.InteropServices;
using Microsoft.Win32.SafeHandles;

internal static class ProbeNative
{
    [DllImport("kernel32", SetLastError = true)]
    internal static extern bool GetNamedPipeClientProcessId(SafePipeHandle pipe, out uint processId);
}
