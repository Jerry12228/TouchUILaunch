namespace Nte.LoginHost;

internal static class LoginTheme
{
    internal static string MascotAsset(string theme) => theme switch
    {
        "night" => "question-mascot-night.png",
        "sunset" => "question-mascot-sunset.png",
        _ => "question-mascot.png"
    };

    internal static string ForLocalTime(DateTime localTime) => localTime.Hour switch
    {
        >= 6 and < 17 => "day",
        >= 17 and < 19 => "sunset",
        _ => "night"
    };
}
