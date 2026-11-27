using System.Security.Cryptography;
using System.Runtime.Versioning;
using System.Text;

namespace Nte.LoginHost.Accounts;

internal static class HelloKeyPolicy
{
    internal static string Name(string userSid, Guid account) => $"{userSid}//TouchUILaunch.NTE/LoginHost/{account:N}";
    internal const string LegacyPrefix = "TouchUILaunch.NTE.LoginHost.Account.v1.";
    internal const int Usage = 3; // NCRYPT_ALLOW_DECRYPT_FLAG | NCRYPT_ALLOW_SIGNING_FLAG; no key import.
    internal const int MandatoryGestureCache = 1;

    [SupportedOSPlatform("windows")]
    internal static CngKeyCreationParameters CreationParameters(nint owner,
        string context = "Protect or unlock the NTE account saved on this device")
    {
        if (owner == 0) throw new InvalidOperationException("An owned UI handle is required for Hello.");
        CngKeyCreationParameters parameters = new()
        {
            Provider = new("Microsoft Passport Key Storage Provider"), ParentWindowHandle = owner
        };
        // CngKey.Create's typed setters persist these properties, which this
        // provider rejects. Provider parameters deliberately use flags zero.
        parameters.Parameters.Add(new("Length", BitConverter.GetBytes(2048), CngPropertyOptions.None));
        parameters.Parameters.Add(new("Key Usage", BitConverter.GetBytes(Usage), CngPropertyOptions.None));
        parameters.Parameters.Add(new("NgcCacheType", BitConverter.GetBytes(MandatoryGestureCache),
            CngPropertyOptions.None));
        parameters.Parameters.Add(
            new("Use Context", Encoding.Unicode.GetBytes(context + "\0"), CngPropertyOptions.None));
        return parameters;
    }
}

internal sealed class HelloProtectionException(string phase, CryptographicException source)
    : CryptographicException($"Windows Hello {phase} failed (0x{source.HResult:X8}).", source)
{
    internal string Phase => phase;
    internal int NativeError => source.HResult;
    internal string LocalExplanation => $"Windows Hello {phase}未完成（0x{source.HResult:X8}），已保存账号仍保留。";
}
