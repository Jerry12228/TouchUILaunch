using System.Text.Json;

namespace Nte.Login;

// Project the PC response into the format observed at the game's SDK callback.
internal static class NteGlobalSdkMetadata
{
    internal static string Extract(NteGlobalSdkQuery query, JsonElement response)
    {
        JsonElement result = NteGlobalAuthentication.RequireResult(response);
        if (query.Notification == 113) return Text(result, "platform");
        if (query.Notification != 104) throw new InvalidOperationException("Unsupported global metadata query.");
        return JsonSerializer.Serialize(new SortedDictionary<string, string>(StringComparer.Ordinal)
        {
            ["City"] = Text(result, "city"),
            ["Country"] = Text(result, "country"),
            ["CountryAbbr"] = Text(result, "countryAbbr"),
            ["CountryCode"] = Text(result, "countryCode"),
            ["IP"] = Text(result, "ip"),
            ["Province"] = Text(result, "province")
        }, NteJsonContext.Default.SortedDictionaryStringString);
    }

    private static string Text(JsonElement value, string name) =>
        value.TryGetProperty(name, out JsonElement field) && field.ValueKind == JsonValueKind.String
            ? field.GetString()! : throw new InvalidDataException("Incomplete global SDK metadata.");
}
