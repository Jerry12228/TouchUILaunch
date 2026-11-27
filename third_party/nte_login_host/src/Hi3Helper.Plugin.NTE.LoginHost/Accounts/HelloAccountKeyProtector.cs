using System.Runtime.Versioning;
using System.Security.Cryptography;
using System.Text;
using System.Security.Principal;

namespace Nte.LoginHost.Accounts;

// Uses only keys created by this component, never the user's system sign-in
// key. A missing provider, rejected gesture policy, cancellation or lost key
// fails closed. There is no software-provider/DPAPI fallback.
[SupportedOSPlatform("windows")]
internal sealed class HelloAccountKeyProtector : IAccountKeyProtector
{
    public string Protection => AccountProtection.Hello;
    private static readonly CngProvider Provider = new("Microsoft Passport Key Storage Provider");
    private static string Name(Guid account) => HelloKeyPolicy.Name(WindowsIdentity.GetCurrent().User!.Value, account);

    public Task<byte[]> WrapAsync(Guid account, byte[] key, nint owner, CancellationToken token) => Task.Run(() =>
    {
        token.ThrowIfCancellationRequested();
        CngKeyCreationParameters options =
            HelloKeyPolicy.CreationParameters(owner, Localization.LoginText.Get("HelloContext"));
        using CngKey cng = Stage("创建保护", () => CngKey.Create(CngAlgorithm.Rsa, Name(account), options));
        SetContext(cng, owner);
        byte[] result = Stage("加密", () => HelloKeyCipher.Encrypt(cng, key));
        if (token.IsCancellationRequested)
        {
            CryptographicOperations.ZeroMemory(result);
            token.ThrowIfCancellationRequested();
        }

        return result;
    }, token);

    public Task<byte[]> UnwrapAsync(Guid account, byte[] wrappedKey, nint owner, CancellationToken token) => Task.Run(
        () =>
        {
            token.ThrowIfCancellationRequested();
            using CngKey cng = Stage("打开保护", () => Open(account));
            SetContext(cng, owner);
            // SDK ncrypt.h: NCRYPT_PIN_CACHE_IS_GESTURE_REQUIRED_PROPERTY,
            // NCRYPT_PIN_CACHE_REQUIRE_GESTURE_FLAG (1). Do not accept a cached PIN.
            Stage("验证策略", () =>
            {
                cng.SetProperty(new CngProperty("PinCacheIsGestureRequired", BitConverter.GetBytes(1),
                    CngPropertyOptions.None));
                return true;
            });
            byte[] plain = Stage("解锁", () => HelloKeyCipher.Decrypt(cng, wrappedKey));
            if (token.IsCancellationRequested)
            {
                CryptographicOperations.ZeroMemory(plain);
                token.ThrowIfCancellationRequested();
            }

            return plain;
        }, token);

    private static void SetContext(CngKey key, nint owner)
    {
        if (owner == 0) throw new InvalidOperationException("An owned UI handle is required for Hello.");
        Stage("保护策略", () =>
        {
            key.ParentWindowHandle = owner;
            key.SetProperty(new CngProperty("Use Context",
                Encoding.Unicode.GetBytes(Localization.LoginText.Get("HelloContext") + "\0"), CngPropertyOptions.None));
            int usage = BitConverter.ToInt32(key.GetProperty("Key Usage", CngPropertyOptions.None).GetValue()!);
            int cache = BitConverter.ToInt32(key.GetProperty("NgcCacheType", CngPropertyOptions.None).GetValue()!);
            if (key.Provider != Provider || key.KeySize != 2048 || (usage & 1) == 0 || (usage & 8) != 0 ||
                cache != HelloKeyPolicy.MandatoryGestureCache)
                throw new CryptographicException("Unexpected account key policy.");
            return true;
        });
    }

    public void Delete(Guid account)
    {
        foreach (string name in new[] { Name(account), HelloKeyPolicy.LegacyPrefix + account.ToString("N") })
        {
            try
            {
                using CngKey key = CngKey.Open(name, Provider, CngKeyOpenOptions.Silent);
                key.Delete();
            }
            catch (CryptographicException ex) when (Missing(ex) ||
                                                    (name.StartsWith(HelloKeyPolicy.LegacyPrefix,
                                                         StringComparison.Ordinal) &&
                                                     ex.HResult == unchecked((int)0x80090027)))
            {
            }
        }
    }

    private static bool Missing(CryptographicException ex) =>
        ex.HResult is unchecked((int)0x80090016) or unchecked((int)0x8009000D) or unchecked((int)0x80090011);

    private static CngKey Open(Guid account)
    {
        try
        {
            return CngKey.Open(Name(account), Provider);
        }
        catch (CryptographicException ex) when (Missing(ex))
        {
            return CngKey.Open(HelloKeyPolicy.LegacyPrefix + account.ToString("N"), Provider);
        }
    }

    private static T Stage<T>(string phase, Func<T> action)
    {
        try
        {
            return action();
        }
        catch (CryptographicException ex) when (ex.HResult is unchecked((int)0x80090036) or unchecked((int)0x800704C7))
        {
            throw new OperationCanceledException("Windows Hello verification was canceled.", ex);
        }
        catch (CryptographicException ex)
        {
            throw new HelloProtectionException(phase, ex);
        }
    }
}
