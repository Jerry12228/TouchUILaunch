using System.Text.Json;

namespace Nte.Login;

// The fixed Windows Global SDK's statistics schema (producer RVA 0x224AD0).
// Device data is session memory, not an HTTP envelope or saved account record.
internal sealed class NteGlobalDeviceInfo
{
    private static readonly string[] Fields =
    [
        "acclrm", "adid", "adm", "afid", "ammr", "anid", "bsid", "bth", "chg", "cmfr", "cpu", "cpuc",
        "cty", "ctz", "ctzid", "cutlz", "dc", "did", "disk", "dmd", "dnm", "dss", "dt", "dtp", "emn",
        "facid", "fdisk", "fmmr", "fps", "gapi", "gpu", "gyro", "iammr", "idfa", "ip", "jb", "lag", "mac",
        "mag", "mbd", "mmr", "ndid", "net", "os", "phn", "pn", "prox", "qqa", "res", "rtbatt", "sdk",
        "soc", "ssc", "ssid", "step", "tchid", "temp", "tnf", "torch", "udid", "udisk", "ummr", "vdid",
        "vmsz", "wba", "wca", "xposed"
    ];
    internal string Json { get; }

    internal NteGlobalDeviceInfo(string deviceId, IReadOnlyDictionary<string, string> measurements, DateTimeOffset now,
        NteOverseasSdkProfile sdk)
    {
        if (!Guid.TryParseExact(deviceId, "N", out _)) throw new ArgumentException("Invalid session device identifier.", nameof(deviceId));
        foreach ((string key, string value) in measurements)
            if (!Fields.Contains(key, StringComparer.Ordinal) || key is "dt" or "did" or "ndid" or "udid" or "sdk" or "os" or "jb" ||
                value.Length > 1024 || value.Contains('\0'))
                throw new ArgumentException("Unsupported device measurement.", nameof(measurements));
        using MemoryStream stream = new();
        using (Utf8JsonWriter json = new(stream))
        {
            json.WriteStartObject();
            foreach (string key in Fields)
            {
                if (key == "dt") { json.WriteNumber(key, now.ToUnixTimeMilliseconds()); continue; }
                string value = key switch
                {
                    "udid" or "did" or "ndid" => deviceId,
                    "sdk" => sdk.DeviceStatisticsVersion,
                    "os" => "4",
                    "jb" => "0", // The vendor's fixed Windows producer value.
                    _ => measurements.TryGetValue(key, out string? measured) ? measured : "NULL"
                };
                json.WriteString(key, value);
            }
            json.WriteEndObject();
        }
        Json = System.Text.Encoding.UTF8.GetString(stream.ToArray());
    }
}
