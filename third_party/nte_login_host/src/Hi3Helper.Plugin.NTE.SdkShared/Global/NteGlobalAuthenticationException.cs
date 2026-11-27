using System.Globalization;
using System.Net;
using System.Net.Mail;
using System.Reflection.PortableExecutable;
using System.Security.Cryptography;
using System.Text;
using System.Text.Json;

namespace Nte.Login;

internal sealed class NteGlobalAuthenticationException(string key, params object[] arguments) : InvalidOperationException(key)
{
    internal string Key { get; } = key;
    internal object[] Arguments { get; } = arguments;
}
