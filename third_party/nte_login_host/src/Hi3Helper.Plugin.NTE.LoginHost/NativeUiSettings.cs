using System.Runtime.InteropServices;
using Avalonia;
using Avalonia.Controls;

namespace Nte.LoginHost;

internal static class NativeUiSettings
{
    internal static bool HighContrast
    {
        get
        {
            HighContrastInfo info = new() { Size = (uint)Marshal.SizeOf<HighContrastInfo>() };
            return SystemParametersInfoW(0x42, info.Size, ref info, 0) && (info.Flags & 1) != 0;
        }
    }
    internal static bool ClientAreaAnimation => SystemParametersInfoW(0x1042, 0, out int enabled, 0) && enabled != 0;
    internal static void SetResourceReference(this StyledElement element, AvaloniaProperty property, string key)
        => element.Bind(property, element.GetResourceObservable(key));

    [StructLayout(LayoutKind.Sequential)]
    private struct HighContrastInfo { internal uint Size, Flags; internal nint Scheme; }
    [DllImport("user32")] private static extern bool SystemParametersInfoW(uint action, uint parameter, ref HighContrastInfo value, uint flags);
    [DllImport("user32")] private static extern bool SystemParametersInfoW(uint action, uint parameter, out int value, uint flags);
}
