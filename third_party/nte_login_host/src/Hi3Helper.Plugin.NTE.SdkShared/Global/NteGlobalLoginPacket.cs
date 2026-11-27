using System.Text.Json;

namespace Nte.Login;

// Verified bridge 2.0.8.0: command 1538 is a length-prefixed UTF-8 JSON
// envelope. It is distinct from mainland command 2 and its fixed fields.
internal static class NteGlobalLoginPacket
{
    internal static byte[][] EncodeSequence(NteGlobalIdentity identity, int hostProcessId)
    {
        // onGlobalOneLoginSuccess supplies UserInfo before LoginFinished.
        // GetUserInfo reads the separate cache populated by command 1539.
        byte[] userInfo = NteGlobalPacket.Encode(1539, hostProcessId, json =>
        {
            json.WriteStartObject(); json.WriteString("result", NteGlobalUserInfo.Encode(identity)); json.WriteEndObject();
        });
        try { return [userInfo, Encode(identity, hostProcessId)]; }
        catch { System.Security.Cryptography.CryptographicOperations.ZeroMemory(userInfo); throw; }
    }
    internal static byte[] Encode(NteGlobalIdentity identity, int hostProcessId)
    {
        return NteGlobalPacket.Encode(1538, hostProcessId, json =>
        {
            json.WriteStartObject(); json.WriteNumber("code", 0); json.WriteString("message", "success");
            json.WritePropertyName("resultObj"); json.WriteStartObject();
            json.WriteString("uid", NteGlobalIdentity.ReadUserId(identity.Result.GetProperty("uid")));
            json.WriteString("token", identity.Result.GetProperty("token").GetString());
            json.WriteNumber("loginType", identity.LoginType);
            json.WriteEndObject(); json.WriteEndObject();
        });
    }
}
