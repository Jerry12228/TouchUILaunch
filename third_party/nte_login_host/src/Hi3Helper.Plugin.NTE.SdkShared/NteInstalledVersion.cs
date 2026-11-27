using System.Xml;

namespace Nte.Login;

internal static class NteInstalledVersion
{
    internal static string Resolve(string gameRoot, NteRegionDefinition region, string officialVersion)
    {
        if (Valid(officialVersion, out string version)) return version;

        string manifest = Path.Combine(gameRoot, ".collapse-nte", "installed", region.AppId, "ResList.xml");
        try
        {
            if (File.Exists(manifest))
            {
                using XmlReader reader = XmlReader.Create(manifest, new XmlReaderSettings
                {
                    DtdProcessing = DtdProcessing.Prohibit, XmlResolver = null,
                    MaxCharactersInDocument = 32 * 1024 * 1024
                });
                if (reader.MoveToContent() == XmlNodeType.Element && reader.Name == "ResList" &&
                    Valid(reader.GetAttribute("version"), out version)) return version;
            }
        }
        catch (Exception error) when (error is IOException or UnauthorizedAccessException or XmlException)
        {
        }

        string marker = Path.Combine(gameRoot, ".collapse_nte_config");
        if (File.Exists(marker) && new FileInfo(marker).Length <= 64 * 1024)
        {
            string? recordedVersion = null, recordedAppId = null;
            foreach (string line in File.ReadLines(marker))
            {
                int separator = line.IndexOf('=');
                if (separator <= 0) continue;
                string key = line[..separator].Trim(), value = line[(separator + 1)..].Trim();
                if (key == "GameVersion") recordedVersion = value;
                else if (key == "AppId") recordedAppId = value;
            }

            // Older China installations have no AppId in this plugin-owned marker.
            if ((recordedAppId == region.AppId || recordedAppId == null && region.Region == NteHostRegion.CN) &&
                Valid(recordedVersion, out version)) return version;
        }

        throw new InvalidDataException("Official public application metadata is missing.");
    }

    private static bool Valid(string? text, out string value)
    {
        value = "";
        if (!Version.TryParse(text?.Trim(), out Version? parsed) ||
            parsed.Major == 0 && parsed.Minor == 0 && parsed.Build <= 0 && parsed.Revision <= 0) return false;
        value = parsed.ToString();
        return true;
    }
}
