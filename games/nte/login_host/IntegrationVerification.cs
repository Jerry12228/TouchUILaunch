using System.Buffers.Binary;
using System.Diagnostics;
using System.Text;
using Nte.Login;

namespace Nte.LoginHost;

internal static class IntegrationVerification
{
    internal static int Run()
    {
        int checks = 0;
        void Check(bool value) { if (!value) throw new InvalidOperationException("Integration fixture failed."); checks++; }
        void Reject(Action action)
        {
            try { action(); } catch (InvalidDataException) { checks++; return; }
            catch (NteGlobalLaunchException) { checks++; return; }
            catch (ArgumentException) { checks++; return; }
            throw new InvalidOperationException("Invalid contract accepted.");
        }
        foreach (var protocol in new[] { NteSdkProtocol.Cn208, NteSdkProtocol.Cn209, NteSdkProtocol.Overseas208, NteSdkProtocol.Overseas209 })
        {
            const int pid = 1234;
            byte[] frame = new byte[1244];
            BinaryPrimitives.WriteUInt32LittleEndian(frame, 0x6B647331);
            BinaryPrimitives.WriteInt32LittleEndian(frame.AsSpan(8), frame.Length);
            Encoding.Unicode.GetBytes(protocol.Application).CopyTo(frame, 12 + 4);
            Encoding.Unicode.GetBytes(protocol.Version).CopyTo(frame, 12 + 0x40C);
            for (int i = 12; i < frame.Length; i++) frame[i] ^= unchecked((byte)pid);
            using var registration = protocol.ReadRegistration(frame, pid);
            Check(registration.Protocol == protocol && registration.ClientId == pid);
            Reject(() => protocol.ReadRegistration(frame, pid + 1).Dispose());
            Reject(() => protocol.ReadRegistration(frame.AsSpan(0, 1243), pid).Dispose());
            frame[4] = 1; Reject(() => protocol.ReadRegistration(frame, pid).Dispose());
        }
        var global = NteOverseasRegion.Global209;
        Check(NteOverseasRegion.Resolve(NteHostRegion.Global, global.ImageSha256.ToLowerInvariant(), global.BridgeSha256) == global);
        Check(global.Protocol == NteSdkProtocol.Overseas209 && global.Sdk.ConfigurationKeyRva == 0xFAD728);
        Reject(() => NteOverseasRegion.Resolve(NteHostRegion.Global, global.ImageSha256, NteOverseasRegion.Global.BridgeSha256));
        Reject(() => NteOverseasRegion.Resolve(NteHostRegion.Taiwan, global.ImageSha256, global.BridgeSha256));
        var options = new NteHostLaunchOptions(@"D:\测试 Client", "-example=\"two words\"", Region: NteHostRegion.Global);
        Check(NteHostLaunchOptions.Parse(options.ToArguments()) == options);
        Reject(() => NteHostLaunchOptions.Parse(["--game-root", options.GameRoot, "--region", "bad"]));
        var plan = new NteOverseasLaunchPlan(options.GameRoot, @"D:\测试 Client\HTGame.exe", options.GameArguments, options.Region);
        Dictionary<string, string> environment = new()
        {
            ["ArcGameAppId"] = "3000001", ["ArcProcessId"] = "1234", ["ArcMoblieNotify"] = "1234",
            ["anchor_w"] = "1", ["anchor_v"] = "2", ["ArcMId"] = "3", ["ArcMMId"] = "4"
        };
        var start = plan.CreateStartInfo("TouchUILaunch.Fixture", "00112233445566778899aabbccddeeff", environment);
        Check(!start.UseShellExecute && start.RedirectStandardOutput && start.RedirectStandardError);
        Check(start.Environment["ArcGameMobileSdkType"] == "2" && start.Environment["ArcGameAppId"] == "3000001");
        Check(start.WorkingDirectory == Path.Combine(options.GameRoot, "Client"));
        environment.Remove("anchor_w");
        Reject(() => plan.CreateStartInfo("TouchUILaunch.Fixture", "00112233445566778899aabbccddeeff", environment));
        Check(!NteLoginPreferences.DefaultPath.Contains("Collapse", StringComparison.OrdinalIgnoreCase));
        Check(!OverseasAccountStore.DirectoryFor(global).Contains("Collapse", StringComparison.OrdinalIgnoreCase));
        Console.WriteLine($"PASS: {checks} SDK integration checks; no game, network or account access.");
        return 0;
    }

    internal static int VerifyInstallation(string root, string regionName)
    {
        var region = Enum.Parse<NteHostRegion>(regionName);
        var options = new NteHostLaunchOptions(root, Region: region);
        if (region == NteHostRegion.CN)
        {
            var selected = NteCnSdkCompatibility.LoadAsync(root).GetAwaiter().GetResult();
            Console.WriteLine("PASS: installed CN SDK profile " + selected.Profile.ProtocolVersion);
        }
        else
        {
            var plan = NteOverseasLaunchPlan.Create(options);
            using var config = NteOverseasApplicationConfiguration.Load(root, region);
            Console.WriteLine("PASS: installed " + config.Region.Name + " SDK " + config.Region.Protocol.Version +
                "; public application configuration validated; no code executed or authentication requested.");
        }
        return 0;
    }

    internal static int OwnedChild(string executable, string folder, string report)
    {
        ProcessStartInfo start = new(executable)
        {
            UseShellExecute = false, WorkingDirectory = folder,
            RedirectStandardOutput = true, RedirectStandardError = true
        };
        foreach (string arg in new[] { "--fixture", "normal", folder, report }) start.ArgumentList.Add(arg);
        start.Environment["TouchUILaunch.ManagedFixture"] = "Unicode 中文";
        using var child = Process.Start(start) ?? throw new InvalidOperationException("Fixture process missing.");
        Task output = child.StandardOutput.ReadToEndAsync(), error = child.StandardError.ReadToEndAsync();
        if (!child.WaitForExit(30000)) { child.Kill(true); throw new TimeoutException(); }
        Task.WaitAll(output, error);
        return child.ExitCode;
    }
}
