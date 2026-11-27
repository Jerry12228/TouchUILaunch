namespace Nte.LoginHost.Accounts;

internal static class AccountLabels
{
    internal static string FromPhone(string phone)
    {
        if (phone.Length != 11 || phone[0] != '1' || phone.Any(c => c is < '0' or > '9'))
            throw new ProbeAuthenticationException("invalid-mainland-phone", null);
        return phone[..3] + "****" + phone[^4..];
    }
}
