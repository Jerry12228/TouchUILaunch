using System.Globalization;
using System.Net.NetworkInformation;
using System.Runtime.InteropServices;
using System.Security;
using Microsoft.Win32;
using Nte.Login;

namespace Nte.LoginHost;

// Reads current Windows hardware through OS APIs. Missing measurements use
// the SDK's NULL convention; no vendor registry/device/account store is read.
internal static class WindowsGlobalDeviceInfo
{
    internal static NteGlobalDeviceInfo Capture(string deviceId, string gameRoot, NteOverseasRegion region)
    {
        Dictionary<string, string> values = new(StringComparer.Ordinal)
        {
            ["dnm"] = Environment.MachineName,
            ["dss"] = Environment.OSVersion.Version.ToString(),
            ["lag"] = CultureInfo.InstalledUICulture.Name.Replace('-', '_'),
            ["cpuc"] = "",
            ["ctz"] = DateTimeOffset.Now.ToString("zzz", CultureInfo.InvariantCulture)
        };
        TimeZoneInfo zone = TimeZoneInfo.Local;
        values["ctzid"] = TimeZoneInfo.TryConvertWindowsIdToIanaId(zone.Id, out string? iana) ? iana : zone.Id;
        try
        {
            values["cty"] = RegionInfo.CurrentRegion.TwoLetterISORegionName;
        }
        catch (ArgumentException)
        {
        }

        ReadCpu(values);
        ReadOs(values);
        ReadDisplay(values);
        ReadNetwork(values);
        MemoryStatus memory = new() { Length = (uint)Marshal.SizeOf<MemoryStatus>() };
        if (GlobalMemoryStatusEx(ref memory))
            values["mmr"] = (memory.TotalPhysical / (1024 * 1024)).ToString(CultureInfo.InvariantCulture);
        try
        {
            DriveInfo drive = new(Path.GetPathRoot(Path.GetFullPath(gameRoot))!);
            if (drive.IsReady)
                values["disk"] = (drive.TotalSize / (1024d * 1024 * 1024)).ToString("F2", CultureInfo.InvariantCulture);
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or ArgumentException)
        {
        }

        return new(deviceId, values, DateTimeOffset.UtcNow, region.Sdk);
    }

    private static void ReadCpu(Dictionary<string, string> values)
    {
        try
        {
            using RegistryKey? key =
                Registry.LocalMachine.OpenSubKey(@"HARDWARE\DESCRIPTION\System\CentralProcessor\0");
            if (key?.GetValue("ProcessorNameString") is string name) values["cpu"] = name.Trim();
            if (key?.GetValue("~MHz") is int mhz && mhz > 0)
                values["cmfr"] = mhz.ToString(CultureInfo.InvariantCulture);
        }
        catch (Exception error) when (error is SecurityException or UnauthorizedAccessException or IOException)
        {
        }
    }

    private static void ReadOs(Dictionary<string, string> values)
    {
        try
        {
            using RegistryKey? key = Registry.LocalMachine.OpenSubKey(@"SOFTWARE\Microsoft\Windows NT\CurrentVersion");
            if (key?.GetValue("ProductName") is string product)
            {
                values["dtp"] = product;
                values["dmd"] = product + " " + Environment.OSVersion.Version;
            }
        }
        catch (Exception error) when (error is SecurityException or UnauthorizedAccessException or IOException)
        {
        }
    }

    private static void ReadDisplay(Dictionary<string, string> values)
    {
        int width = GetSystemMetrics(0), height = GetSystemMetrics(1);
        if (width > 0 && height > 0) values["res"] = FormattableString.Invariant($"{width}x{height}");
        for (uint i = 0; i < 16; i++)
        {
            DisplayDevice display = new() { Size = (uint)Marshal.SizeOf<DisplayDevice>() };
            if (!EnumDisplayDevicesW(null, i, ref display, 0)) break;
            if ((display.StateFlags & 1) != 0 && !string.IsNullOrWhiteSpace(display.Description))
            {
                values["gpu"] = display.Description;
                break;
            }
        }
    }

    private static void ReadNetwork(Dictionary<string, string> values)
    {
        try
        {
            NetworkInterface? device = NetworkInterface.GetAllNetworkInterfaces().FirstOrDefault(item =>
                item.OperationalStatus == OperationalStatus.Up &&
                item.NetworkInterfaceType is NetworkInterfaceType.Ethernet or NetworkInterfaceType.Wireless80211 &&
                item.GetPhysicalAddress().GetAddressBytes().Length == 6);
            if (device == null) return;
            values["net"] = device.NetworkInterfaceType == NetworkInterfaceType.Wireless80211 ? "wLan" : "Ethernet";
            values["mac"] = string.Join(":",
                device.GetPhysicalAddress().GetAddressBytes()
                    .Select(value => value.ToString("X2", CultureInfo.InvariantCulture)));
        }
        catch (NetworkInformationException)
        {
        }
    }

    [StructLayout(LayoutKind.Sequential)]
    private struct MemoryStatus
    {
        internal uint Length, Load;

        internal ulong TotalPhysical,
            AvailablePhysical,
            TotalPageFile,
            AvailablePageFile,
            TotalVirtual,
            AvailableVirtual,
            AvailableExtendedVirtual;
    }

    [StructLayout(LayoutKind.Sequential, CharSet = CharSet.Unicode)]
    private struct DisplayDevice
    {
        internal uint Size;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 32)]
        internal string Name;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        internal string Description;

        internal uint StateFlags;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        internal string Id;

        [MarshalAs(UnmanagedType.ByValTStr, SizeConst = 128)]
        internal string Key;
    }

    [DllImport("kernel32", SetLastError = true)]
    private static extern bool GlobalMemoryStatusEx(ref MemoryStatus status);

    [DllImport("user32")]
    private static extern int GetSystemMetrics(int index);

    [DllImport("user32", CharSet = CharSet.Unicode)]
    private static extern bool EnumDisplayDevicesW(string? device, uint number, ref DisplayDevice display, uint flags);
}
