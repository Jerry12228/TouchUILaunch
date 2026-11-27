using System.IO.Pipes;
using System.Security.AccessControl;
using System.Security.Principal;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace Nte.LoginHost;

internal static class BrowserCallbackPipe
{
    internal static NamedPipeServerStream Create(string name)
    {
        // CurrentUserOnly compares elevation levels on Windows. Browser
        // callbacks run unelevated while the game host may be elevated. Grant
        // this user's SID explicitly instead of granting an elevation group.
        using WindowsIdentity identity = WindowsIdentity.GetCurrent();
        SecurityIdentifier user = identity.User ?? throw new InvalidOperationException("Windows user unavailable.");
        PipeSecurity security = new(); security.SetAccessRuleProtection(true, false); security.SetOwner(user);
        security.AddAccessRule(new(user, PipeAccessRights.FullControl, AccessControlType.Allow));
        security.AddAccessRule(new(new SecurityIdentifier(WellKnownSidType.LocalSystemSid, null), PipeAccessRights.FullControl, AccessControlType.Allow));
        NamedPipeServerStream pipe = NamedPipeServerStreamAcl.Create(name, PipeDirection.In, 1, PipeTransmissionMode.Byte,
            PipeOptions.Asynchronous | PipeOptions.FirstPipeInstance, 8192, 0, security);
        try
        {
            // Collapse installs plugins under LocalLow. Windows consequently
            // starts a browser-launched apphost at low integrity even when the
            // existing game host is medium/high. The user-only DACL above is
            // still required; lower only this callback pipe's mandatory label.
            AllowLowIntegrityCallback(pipe);
            return pipe;
        }
        catch { pipe.Dispose(); throw; }
    }

    private static void AllowLowIntegrityCallback(NamedPipeServerStream pipe)
    {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW("S:(ML;;NW;;;LW)", 1, out nint descriptor, out _))
            throw new Win32Exception(Marshal.GetLastWin32Error());
        try
        {
            if (!GetSecurityDescriptorSacl(descriptor, out bool present, out nint sacl, out _) || !present)
                throw new Win32Exception(Marshal.GetLastWin32Error());
            uint result = SetSecurityInfo(pipe.SafePipeHandle.DangerousGetHandle(), 6, 0x10, 0, 0, 0, sacl);
            if (result != 0) throw new Win32Exception((int)result);
        }
        finally { LocalFree(descriptor); }
    }

    [DllImport("advapi32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool ConvertStringSecurityDescriptorToSecurityDescriptorW(string sddl, uint revision, out nint descriptor, out uint size);
    [DllImport("advapi32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetSecurityDescriptorSacl(nint descriptor, [MarshalAs(UnmanagedType.Bool)] out bool present, out nint sacl, [MarshalAs(UnmanagedType.Bool)] out bool defaulted);
    [DllImport("advapi32.dll")]
    private static extern uint SetSecurityInfo(nint handle, uint type, uint information, nint owner, nint group, nint dacl, nint sacl);
    [DllImport("kernel32.dll")]
    private static extern nint LocalFree(nint memory);
    internal static bool IsOwnedByCurrentUser(NamedPipeClientStream pipe)
    {
        using WindowsIdentity identity = WindowsIdentity.GetCurrent();
        return identity.User is SecurityIdentifier user &&
               pipe.GetAccessControl().GetOwner(typeof(SecurityIdentifier)) is SecurityIdentifier owner && owner.Equals(user);
    }
}
