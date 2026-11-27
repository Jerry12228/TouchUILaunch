using System.Globalization;
using System.Resources;

namespace Nte.LoginHost.Localization;

internal static class LoginText
{
    private static readonly ResourceManager Resources = new("Nte.LoginHost.Localization.Strings",
        typeof(LoginText).Assembly);

    internal static CultureInfo Culture { get; private set; } = CultureInfo.GetCultureInfo("zh-CN");
    internal static string WebLanguage => Culture.Name.ToLowerInvariant();

    internal static void UseLanguage(string? language)
    {
        string normalized = language?.Replace('_', '-').ToLowerInvariant() ?? "en";
        string value = normalized.StartsWith("zh", StringComparison.Ordinal)
            ? normalized.Contains("hant", StringComparison.Ordinal) || normalized is "zh-tw" or "zh-hk" or "zh-mo"
                ? "zh-TW"
                : "zh-CN"
            : normalized.Split('-')[0];
        Culture = CultureInfo.GetCultureInfo(
            value is "zh-CN" or "zh-TW" or "en" or "ja" or "ko" or "de" or "fr" or "ru" or "es" ? value : "en");
        CultureInfo.DefaultThreadCurrentUICulture = Culture;
        CultureInfo.CurrentUICulture = Culture;
    }

    internal static string Get(string key, params object[] arguments)
    {
        string text = Resources.GetString(key, Culture) ??
                      throw new MissingManifestResourceException("Missing login text: " + key);
        return arguments.Length == 0 ? text : string.Format(Culture, text, arguments);
    }

    // Execute in the published binary so this also verifies AOT's embedded
    // satellite resources, rather than loading a managed assembly externally.
    internal static int VerifyCatalogs()
    {
        ResourceSet baseline = Resources.GetResourceSet(CultureInfo.InvariantCulture, true, false)!;
        string[] keys = baseline.Cast<System.Collections.DictionaryEntry>().Select(entry => (string)entry.Key).Order()
            .ToArray();
        foreach (string language in new[] { "en", "zh-CN", "zh-TW", "ja", "ko", "de", "fr", "ru", "es" })
        {
            CultureInfo culture =
                language == "en" ? CultureInfo.InvariantCulture : CultureInfo.GetCultureInfo(language);
            ResourceSet catalog = Resources.GetResourceSet(culture, true, false) ??
                                  throw new MissingManifestResourceException(language);
            foreach (string key in keys)
            {
                string value = catalog.GetString(key) ??
                               throw new MissingManifestResourceException(language + "/" + key);
                if (string.IsNullOrWhiteSpace(value)) throw new MissingManifestResourceException(language + "/" + key);

                string Placeholders(string text) => string.Join(",",
                    System.Text.RegularExpressions.Regex.Matches(text, @"\{\d+\}").Select(match => match.Value)
                        .Order());

                if (Placeholders(value) != Placeholders(baseline.GetString(key)!))
                    throw new InvalidDataException(language + "/" + key);
            }
        }

        return 0;
    }
}
