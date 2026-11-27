using System.Buffers.Binary;
using System.Diagnostics;
using System.IO.Pipes;
using System.Security.Cryptography;
using Nte.Login;

namespace Nte.LoginHost;

// Owns the game and global SDK session without an official platform process.
internal sealed class OverseasGameLoginHost(NteHostLaunchOptions options, LoginWindow window) : IDisposable
{
    private readonly NteOverseasRegion region = NteOverseasRegion.ForInstallation(options.Region, options.GameRoot);
    private readonly CancellationTokenSource lifetime = new();
    private ProbeEnvironment? context;
    private NteOverseasApplicationConfiguration? configuration;
    private NteGlobalAuthentication? authentication;
    private Task logins = Task.CompletedTask;
    private Task metadata = Task.CompletedTask;
    private GlobalSdkQueries? sdkQueries;
    private readonly NteGlobalSession session = new();
    private OverseasAccountStore? accounts;
    private bool automaticRestoreAllowed = true;
    private Task logoutSelection = Task.CompletedTask;

    internal async Task<bool> RunAsync()
    {
        using Mutex instance = new(false, @"Local\TouchUILaunch.NTE.LoginHost." + region.AppId, out bool created);
        if (!created) throw new NteGlobalLaunchException("GlobalAlreadyRunning");
        foreach (string name in new[] { "HTGame", "NTEGame", "NTEGlobalGame", "NTEGlobalUpdate", "NTETWGame", "NTETWUpdate" })
        {
            Process[] running = Process.GetProcessesByName(name);
            try
            {
                if (running.Length > 0) throw new NteGlobalLaunchException("GlobalAlreadyRunning");
            }
            finally
            {
                foreach (Process process in running) process.Dispose();
            }
        }

        NteOverseasLaunchPlan plan = NteOverseasLaunchPlan.Create(options);
        string root = Path.GetFullPath(options.GameRoot);
        await using (FileStream bridge = File.OpenRead(Path.Combine(root, region.LauncherDirectory, "WPGameClientSDK_64.dll")))
            if (Convert.ToHexString(await SHA256.HashDataAsync(bridge)) !=
                region.BridgeSha256)
                throw new NteGlobalLaunchException("GlobalSdkUnsupported");
        configuration = NteOverseasApplicationConfiguration.Load(root, options.Region);
        NteHostStartupDiagnostics.Phase("compatibility-accepted");
        context = new(root, true, useOwnExecutable: true, hostExecutable: Environment.ProcessPath!,
            launcherDirectoryName: region.LauncherDirectory, applicationId: region.AppId);
        context.GameWindowChanged += window.NotifySdkGeometryChanged;
        context.LoginRequested += window.NotifySdkLoginRequested;
        // The global SDK uses 101 for logout and 102 for account switching.
        // Both notifications already clear its local identity before posting.
        context.LogoutRequested += () => Avalonia.Threading.Dispatcher.UIThread.Post(() =>
        {
            automaticRestoreAllowed = false;
            session.Clear();
            window.CancelForGameExit();
            logoutSelection = ClearRememberedSelectionAsync();
        });
        accounts = new(OverseasAccountStore.DirectoryFor(region), new Accounts.WindowsAccountKeyProtector(),
            new Accounts.HelloAccountKeyProtector(), region);
        string deviceId = await accounts.DeviceIdAsync(lifetime.Token);
        authentication = new(configuration, context.InstalledVersion, language: Localization.LoginText.WebLanguage,
            deviceId: deviceId);
        sdkQueries = new(authentication, window);
        context.GlobalQueryRequested += sdkQueries.Enqueue;
        string pipeName = "TouchUILaunch.NTE." + region.Name + ".Login." + Guid.NewGuid().ToString("N");
        using NamedPipeServerStream pipe = new(pipeName, PipeDirection.InOut, 1, PipeTransmissionMode.Message,
            PipeOptions.Asynchronous | PipeOptions.CurrentUserOnly);
        ProcessStartInfo start =
            plan.CreateStartInfo(pipeName, authentication.DeviceId, context.GetEnvironmentValues());
        Process? launched;
        try
        {
            PatcherLaunchEnvironment.TryApply(start.Environment, root, context.ApplicationId, region.LauncherDirectory);
            NteGlobalGameEnvironment.Apply(start, configuration);
            launched = Process.Start(start);
        }
        finally
        {
            NteGlobalGameEnvironment.Clear(start);
        }

        using Process game = launched ?? throw new NteGlobalLaunchException("GlobalStartFailed");
        NteHostStartupDiagnostics.Phase("owned-game-created");
        Task exit = WatchExitAsync(game);
        Task output = Task.CompletedTask, errors = Task.CompletedTask;
        return await GameStartedLifetime.RunAsync(exit, async () =>
        {
            output = DrainAsync(game.StandardOutput.BaseStream);
            errors = DrainAsync(game.StandardError.BaseStream);
            try
            {
                game.PriorityBoostEnabled = options.PriorityBoost;
                game.PriorityClass = options.Priority;
            }
            catch (Exception ex) when (ex is System.ComponentModel.Win32Exception or InvalidOperationException)
            {
            }

            window.Attach(game.Id);
            await ServeAsync(pipe, game.Id);
        }, async _ =>
        {
            window.CancelForGameExit();
            await Task.WhenAny(window.ShowFailureAsync(Localization.LoginText.Get("GlobalCommunicationFailed")), exit);
            if (!exit.IsCompleted) window.Hide();
        }, () => lifetime.IsCancellationRequested, async () =>
        {
            lifetime.Cancel();
            try
            {
                await Task.WhenAll(logins, metadata);
            }
            catch (OperationCanceledException)
            {
            }

            await Task.WhenAll(output, errors);
        });
    }

    private async Task ServeAsync(NamedPipeServerStream pipe, int clientId)
    {
        bool registered = false;
        string? application = null;
        while (!lifetime.IsCancellationRequested)
        {
            await pipe.WaitForConnectionAsync(lifetime.Token);
            try
            {
                if (!ProbeNative.GetNamedPipeClientProcessId(pipe.SafePipeHandle, out uint peer) ||
                    peer != clientId) continue;
                while (pipe.IsConnected && !lifetime.IsCancellationRequested)
                {
                    byte[] frame = new byte[65536];
                    try
                    {
                        int count = 0, read;
                        do
                        {
                            read = await pipe.ReadAsync(frame.AsMemory(count), lifetime.Token);
                            count += read;
                            if (count == frame.Length && !pipe.IsMessageComplete)
                                throw new InvalidDataException("Global SDK frame size bound.");
                        } while (read > 0 && !pipe.IsMessageComplete);

                        if (count == 0) break;
                        if (count < 12 || BinaryPrimitives.ReadUInt32LittleEndian(frame) != 0x6B647331 ||
                            BinaryPrimitives.ReadInt32LittleEndian(frame.AsSpan(8)) != count)
                            throw new InvalidDataException("Global SDK frame header mismatch.");
                        uint command = BinaryPrimitives.ReadUInt32LittleEndian(frame.AsSpan(4));
                        if (command == 1000 && count == 796 &&
                            BinaryPrimitives.ReadUInt32LittleEndian(frame.AsSpan(12)) == clientId)
                        {
                            application = System.Text.Encoding.Unicode.GetString(frame, 18, 64).Split('\0')[0];
                            if (application != "global")
                                throw new InvalidDataException("Unexpected global SDK application.");
                            await pipe.WriteAsync(context!.CreateAcknowledgement(frame.AsSpan(0, count)),
                                lifetime.Token);
                            await pipe.FlushAsync(lifetime.Token);
                            NteHostStartupDiagnostics.Phase("sdk-environment-acknowledged");
                        }
                        else if (command == 0 && count == 1244 && !registered && application == "global")
                        {
                            // Retain the registered window/version/peer checks,
                            // then use the observed Global duplex reply path.
                            using NteSdkRegistration registration = region.Protocol.ReadRegistration(frame.AsSpan(0, count), clientId);
                            NteHostStartupDiagnostics.Phase("sdk-registration-validated");
                            _ = ProbeCallbackTransport.FromRegistration(registration, context!.WindowHandle);
                            NteGlobalPipeTransport callback = new(pipe, region);
                            nint gameWindow = await WaitForGameWindowAsync(clientId, lifetime.Token);
                            // Show first-use disclosure before supplying device
                            // measurements or allowing any automatic account login.
                            if (!await window.EnsureNoticeAsync(true, lifetime.Token)) return;
                            await callback.BindGameWindowAsync(gameWindow, clientId, lifetime.Token);
                            await callback.SendWindowsPlatformAsync(lifetime.Token);
                            await callback.SendLanguageAsync(region.HostRegion == NteHostRegion.Taiwan ? "zh-TW" : Localization.LoginText.Culture.Name, lifetime.Token);
                            await callback.SendClientContextAsync(lifetime.Token);
                            NteGlobalDeviceInfo device = await Task.Run(
                                () => WindowsGlobalDeviceInfo.Capture(authentication!.DeviceId, options.GameRoot, region),
                                lifetime.Token);
                            await callback.SendDeviceInfoAsync(device, lifetime.Token);
                            registered = true;
                            NteHostStartupDiagnostics.Phase("sdk-device-context-delivered");
                            logins = RunLoginsAsync(callback);
                            metadata = sdkQueries!.RunAsync(callback, lifetime.Token);
                        }
                    }
                    finally
                    {
                        CryptographicOperations.ZeroMemory(frame);
                    }
                }
            }
            catch (InvalidDataException)
            {
                NteHostStartupDiagnostics.Phase("sdk-registration-rejected");
                throw;
            }
            catch (IOException) when (!registered && !lifetime.IsCancellationRequested)
            {
            }
            finally
            {
                if (pipe.IsConnected) pipe.Disconnect();
            }

            if (registered && !lifetime.IsCancellationRequested)
                throw new IOException("Registered Global SDK connection ended.");
        }
    }

    private async Task RunLoginsAsync(NteGlobalPipeTransport callback)
    {
        try
        {
            while (!lifetime.IsCancellationRequested)
            {
                await context!.WaitForNextLoginRequestAsync(lifetime.Token);
                NteHostStartupDiagnostics.Phase("login-requested");
                bool consumed = false;
                CancellationToken sessionToken = session.Token;
                try
                {
                    if (!await window.EnsureNoticeAsync(true, lifetime.Token)) continue;

                    async Task DeliverAsync(NteGlobalIdentity identity, CancellationToken token)
                    {
                        token.ThrowIfCancellationRequested();
                        sessionToken.ThrowIfCancellationRequested();
                        context.CompleteLoginRequest();
                        consumed = true;
                        await callback.SendLoginAsync(identity, token);
                        NteHostStartupDiagnostics.Phase("authenticated-identity-delivered");
                        session.Remember(identity, sessionToken);
                    }

                    using NteGlobalIdentity? saved = session.Capture();
                    if (saved != null)
                    {
                        try
                        {
                            using CancellationTokenSource restore =
                                CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token, sessionToken);
                            using NteGlobalIdentity restored =
                                await authentication!.RestoreSessionAsync(saved, restore.Token);
                            await DeliverAsync(restored, restore.Token);
                            continue;
                        }
                        catch (NteGlobalAuthenticationException ex) when (ex.Key == "ServiceRejected")
                        {
                            session.Clear();
                            sessionToken = session.Token;
                        }
                    }

                    string? notice = null;
                    if (automaticRestoreAllowed)
                    {
                        automaticRestoreAllowed = false;
                        try
                        {
                            await logoutSelection;
                            if (await accounts!.LastAsync(lifetime.Token) is { } last)
                            {
                                using CancellationTokenSource restore =
                                    CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token, sessionToken);
                                bool useHello = OverseasAccountStore.ReadProtectionPreference(region);
                                await window.WithGlobalAccountOwnerAsync(useHello || accounts.RequiresHello(last.Id),
                                    owner =>
                                        accounts.RestoreAndDeliverAsync(last.Id, authentication!, true, owner,
                                            DeliverAsync, restore.Token, useHello));
                                continue;
                            }
                        }
                        catch (OperationCanceledException) when (!lifetime.IsCancellationRequested &&
                                                                 !sessionToken.IsCancellationRequested)
                        {
                            notice = Localization.LoginText.Get("AuthorizationCancelled");
                        }
                        catch (Exception ex) when (ex is NteGlobalAuthenticationException or HttpRequestException
                                                       or IOException or
                                                       CryptographicException or System.Text.Json.JsonException
                                                       or InvalidOperationException or UnauthorizedAccessException)
                        {
                            notice = Localization.LoginText.Get("GlobalSavedUnavailable");
                        }
                    }

                    using CancellationTokenSource form =
                        CancellationTokenSource.CreateLinkedTokenSource(lifetime.Token, sessionToken);
                    await window.RequestGlobalAsync(authentication!, DeliverAsync, form.Token, accounts,
                        notice: notice);
                }
                catch (OperationCanceledException) when (!lifetime.IsCancellationRequested)
                {
                }
                catch (NteGlobalAuthenticationException ex)
                {
                    await window.ShowFailureAsync(Localization.LoginText.Get(ex.Key, ex.Arguments))
                        .WaitAsync(lifetime.Token);
                }
                catch (Exception ex) when (ex is HttpRequestException or IOException)
                {
                    await window.ShowFailureAsync(Localization.LoginText.Get("GlobalSignInFailed"))
                        .WaitAsync(lifetime.Token);
                }
                finally
                {
                    if (!consumed) context.CompleteLoginRequest();
                }
            }
        }
        catch (OperationCanceledException) when (lifetime.IsCancellationRequested)
        {
        }
        catch (Exception) when (!lifetime.IsCancellationRequested)
        {
            // An asynchronous UI failure must become visible while the game
            // remains owned; do not silently leave a faulted login task.
            await window.ShowFailureAsync(Localization.LoginText.Get("GlobalSignInFailed")).WaitAsync(lifetime.Token);
        }
    }

    private async Task ClearRememberedSelectionAsync()
    {
        try
        {
            if (accounts != null) await accounts.ClearRememberedAsync(lifetime.Token);
        }
        catch (Exception ex) when (ex is IOException or CryptographicException or UnauthorizedAccessException)
        {
            window.ReportServiceFailure(Localization.LoginText.Get("LogoutStateNotSaved"));
        }
    }

    private async Task WatchExitAsync(Process game)
    {
        await game.WaitForExitAsync();
        lifetime.Cancel();
        Avalonia.Threading.Dispatcher.UIThread.Post(window.CancelForGameExit);
    }

    private static async Task<nint> WaitForGameWindowAsync(int clientId, CancellationToken token)
    {
        using CancellationTokenSource timeout = CancellationTokenSource.CreateLinkedTokenSource(token);
        timeout.CancelAfter(TimeSpan.FromSeconds(45));
        while (true)
        {
            nint target = WindowNative.FindGameWindow(clientId);
            if (target != 0) return target;
            await Task.Delay(100, timeout.Token);
        }
    }

    private async Task DrainAsync(Stream stream)
    {
        try
        {
            await stream.CopyToAsync(Stream.Null, lifetime.Token);
        }
        catch (OperationCanceledException) when (lifetime.IsCancellationRequested)
        {
        }
        catch (IOException) when (lifetime.IsCancellationRequested)
        {
        }
    }

    public void Dispose()
    {
        lifetime.Cancel();
        session.Dispose();
        authentication?.Dispose();
        configuration?.Dispose();
        context?.Dispose();
        lifetime.Dispose();
    }
}
