using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;

namespace Nte.Login;

internal enum NteHostRegion { CN, Global, Taiwan }

internal sealed record NteHostLaunchOptions(string GameRoot, string? AdditionalArguments = null,
    bool PriorityBoost = false, ProcessPriorityClass Priority = ProcessPriorityClass.Normal,
    NteHostRegion Region = NteHostRegion.CN, string Language = "zh-CN")
{
    internal const string RequiredGameArguments = NteRegionDefinition.RequiredGameArguments;
    internal string GameArguments => string.IsNullOrWhiteSpace(AdditionalArguments) ? RequiredGameArguments : RequiredGameArguments + " " + AdditionalArguments;

    internal string[] ToArguments()
    {
        Validate();
        return ["--game-root", Path.GetFullPath(GameRoot), "--game-arguments", AdditionalArguments ?? "",
            "--priority", Priority.ToString(), "--priority-boost", PriorityBoost.ToString(), "--region", Region.ToString(), "--language", Language];
    }

    internal static NteHostLaunchOptions Parse(IReadOnlyList<string> arguments)
    {
        Dictionary<string, string> fields = new(StringComparer.Ordinal);
        if (arguments.Count % 2 != 0) throw new ArgumentException("Login host arguments must be option/value pairs.");
        for (int i = 0; i < arguments.Count; i += 2)
        {
            if (arguments[i] is not ("--game-root" or "--game-arguments" or "--priority" or "--priority-boost" or "--region" or "--language") || !fields.TryAdd(arguments[i], arguments[i + 1]))
                throw new ArgumentException("Unknown or duplicate login host option.");
        }
        if (!fields.TryGetValue("--game-root", out string? root)) throw new ArgumentException("Game root is required.");
        ProcessPriorityClass priority = ProcessPriorityClass.Normal; bool boost = false;
        if (fields.TryGetValue("--priority", out string? priorityText) && (!Enum.TryParse(priorityText, out priority) || !Enum.IsDefined(priority)))
            throw new ArgumentException("Invalid game priority.");
        if (fields.TryGetValue("--priority-boost", out string? boostText) && !bool.TryParse(boostText, out boost))
            throw new ArgumentException("Invalid game priority boost.");
        NteHostRegion region = NteHostRegion.CN;
        if (fields.TryGetValue("--region", out string? regionText) &&
            (regionText is not ("CN" or "Global" or "Taiwan") || !Enum.TryParse(regionText, out region)))
            throw new ArgumentException("Invalid game region.");
        NteHostLaunchOptions options = new(root, fields.GetValueOrDefault("--game-arguments"), boost, priority, region, fields.GetValueOrDefault("--language") ?? "zh-CN");
        options.Validate(); return options;
    }

    private void Validate()
    {
        if (string.IsNullOrWhiteSpace(GameRoot) || !Path.IsPathFullyQualified(GameRoot) || GameRoot.Contains('\0') ||
            (AdditionalArguments?.Length ?? 0) > 8192 || AdditionalArguments?.Contains('\0') == true || !Enum.IsDefined(Priority) || !Enum.IsDefined(Region) ||
            string.IsNullOrWhiteSpace(Language) || Language.Length > 35 || !System.Linq.Enumerable.All(Language, c => char.IsAsciiLetterOrDigit(c) || c is '-' or '_'))
            throw new ArgumentException("Invalid login host launch context.");
    }
}
