using System.Text.Json;

internal enum ProbeChannelValidationStatus { Validated, ServiceRejected, NetworkFailure, UnknownResponse, RequiresOfficialInteraction, RequestRejected }

internal readonly record struct ProbeChannelValidationResult(ProbeChannelValidationStatus Status, int? Code)
{
    internal static ProbeChannelValidationResult Parse(JsonElement root)
    {
        if (root.ValueKind != JsonValueKind.Object || root.EnumerateObject().Count(p => p.NameEquals("code")) != 1 ||
            !root.TryGetProperty("code", out JsonElement codeField) || codeField.ValueKind != JsonValueKind.Number || !codeField.TryGetInt32(out int code))
            return new(ProbeChannelValidationStatus.UnknownResponse, null);
        if (code != 0) return new(ProbeChannelValidationStatus.ServiceRejected, code);
        if (root.EnumerateObject().Count(p => p.NameEquals("result")) > 1) return new(ProbeChannelValidationStatus.UnknownResponse, code);
        if (root.TryGetProperty("result", out JsonElement result) && result.ValueKind != JsonValueKind.Null)
        {
            if (result.ValueKind != JsonValueKind.Object) return new(ProbeChannelValidationStatus.UnknownResponse, code);
            // The fixed callback can request a privacy-agreement interaction.
            // Until its complete contract is verified, do not silently consent.
            if (result.EnumerateObject().Any(p => p.NameEquals("privacyAgreement") && p.Value.ValueKind != JsonValueKind.Null))
                return new(ProbeChannelValidationStatus.RequiresOfficialInteraction, code);
        }
        // Validation confirms the existing channel identity. It never supplies
        // a replacement token, even if unrelated response fields look like one.
        return new(ProbeChannelValidationStatus.Validated, code);
    }

    internal void RequireValidated()
    {
        if (Status == ProbeChannelValidationStatus.Validated) return;
        throw new ProbeAuthenticationException(Status switch
        {
            ProbeChannelValidationStatus.ServiceRejected => "channel-validation-rejected",
            ProbeChannelValidationStatus.NetworkFailure => "channel-validation-network-failure",
            ProbeChannelValidationStatus.RequiresOfficialInteraction => "channel-validation-official-interaction",
            ProbeChannelValidationStatus.RequestRejected => "channel-validation-request-rejected",
            _ => "channel-validation-unknown-response"
        }, Code);
    }
}
