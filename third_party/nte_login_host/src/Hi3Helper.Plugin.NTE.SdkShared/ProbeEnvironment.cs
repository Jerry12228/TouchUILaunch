using System.Buffers.Binary;
using System.Globalization;
using System.IO.MemoryMappedFiles;
using System.Text;
using System.Text.RegularExpressions;
using Nte.Login;

internal sealed class ProbeEnvironment : IDisposable
{
    private readonly ProbeWindow window;
    private readonly string mapName = "Local\\TouchUILaunch.NTE.SdkProbe.Map." + Guid.NewGuid().ToString("N");
    private readonly string mutexName = "Local\\TouchUILaunch.NTE.SdkProbe.Mutex." + Guid.NewGuid().ToString("N");
    private readonly MemoryMappedFile map;
    private readonly Mutex mutex;
    private readonly string appId;
    private readonly string version;
    private readonly string launcher;
    private readonly string[] platformExecutables;
    private readonly bool hostContext;
    private readonly bool startupContext;
    private readonly EventWaitHandle? readyEvent;
    private readonly EventWaitHandle? contextEvent;
    public IEnumerable<object> Messages => window.Messages;
    internal nint WindowHandle => window.Handle;
    public string InstalledVersion => version;
    internal string ApplicationId => appId;
    internal Task WaitForLoginRequestAsync(CancellationToken token) => window.WaitForLoginRequestAsync(token);
    internal Task WaitForNextLoginRequestAsync(CancellationToken token) => window.WaitForNextLoginRequestAsync(token);
    internal void CompleteLoginRequest() => window.CompleteLoginRequest();

    internal event Action<Nte.Login.NteGlobalSdkQuery>? GlobalQueryRequested
    {
        add => window.GlobalQueryRequested += value;
        remove => window.GlobalQueryRequested -= value;
    }

    internal event Action? GameWindowChanged
    {
        add => window.GameWindowChanged += value;
        remove => window.GameWindowChanged -= value;
    }

    internal event Action? LoginRequested
    {
        add => window.LoginRequested += value;
        remove => window.LoginRequested -= value;
    }

    internal event Action? LogoutRequested
    {
        add => window.LogoutRequested += value;
        remove => window.LogoutRequested -= value;
    }

    public ProbeEnvironment(string gameRoot, bool supplyHostContext, bool supplyStartupContext = false,
        bool useOwnExecutable = false,
        string? hostExecutable = null, string launcherDirectoryName = NteRegionDefinition.CnLauncherDirectory,
        string? applicationId = null)
    {
        NteRegionDefinition definition = NteRegionDefinition.FromLauncherDirectory(launcherDirectoryName);
        if (applicationId != null && applicationId != definition.AppId)
            throw new ArgumentException("Application and launcher directory belong to different regions.",
                nameof(applicationId));
        hostContext = supplyHostContext;
        startupContext = supplyStartupContext;
        launcher = Path.Combine(gameRoot, launcherDirectoryName);
        // These are SDK platform path references, not executables the probe
        // starts. The isolated candidate tests whether the SDK can use this
        // host's own apphost for all roles without requiring vendor EXEs.
        string ownExecutable = hostExecutable ?? Path.Combine(AppContext.BaseDirectory, "NteSdkProbe.exe");
        platformExecutables = useOwnExecutable
            ? [ownExecutable, ownExecutable, ownExecutable]
            :
            [
                Path.Combine(launcher, "NTELauncher.exe"), Path.Combine(launcher, "NTEGame.exe"),
                Path.Combine(launcher, "NTEUpdate.exe")
            ];
        string configuration = File.ReadAllText(Path.Combine(launcher, "Config/Config.ini"));
        appId = applicationId ?? Regex.Match(configuration, @"(?m)^OneAppId=(\d+)\s*$").Groups[1].Value;
        string patcherPath = Path.Combine(launcher, NteRegionDefinition.PatcherSdkRelativePath, "config.xml");
        string patcher = File.Exists(patcherPath) ? File.ReadAllText(patcherPath) : "";
        if (appId != definition.AppId)
            throw new InvalidDataException("Official public application metadata is missing.");
        version = NteInstalledVersion.Resolve(gameRoot, definition,
            Regex.Match(patcher, @"<ResVersion>(\d+(?:\.\d+)+)</ResVersion>").Groups[1].Value);
        window = new(definition.LoginNotification);
        // The probe owns these objects exclusively. The baseline table is empty;
        // host-context mode supplies public application metadata below. Neither
        // mode supplies account data, an SDK result or an authentication token.
        map = MemoryMappedFile.CreateNew(mapName, 1024 * 1024);
        mutex = new Mutex(false, mutexName);
        if (hostContext)
        {
            foreach (string path in platformExecutables)
                if (!File.Exists(path))
                    throw new FileNotFoundException("Existing SDK platform path is missing.", path);
            // A readiness signal for this host is separate from an SDK init or
            // login result. Never reuse another process's event.
            string readyName = @"Global\CoreClient_StartEvent" + (startupContext ? "" : "_" + appId);
            readyEvent = new EventWaitHandle(true, EventResetMode.ManualReset, readyName, out bool created);
            if (!created)
            {
                readyEvent.Dispose();
                throw new IOException("A platform readiness event already exists.");
            }

            string contextName = Guid.NewGuid().ToString("N");
            contextEvent = new EventWaitHandle(false, EventResetMode.ManualReset, contextName, out created);
            if (!created) throw new IOException("A context event already exists.");
            using MemoryMappedViewAccessor view = map.CreateViewAccessor();
            view.Write(0, 1u);
            view.Write(8, uint.Parse(appId, CultureInfo.InvariantCulture));
            view.Write(4 + 8, unchecked((ushort)Environment.ProcessId));
            // The official row's first field is a 32-character identifier, not
            // an install path. These names belong to this diagnostic host.
            WriteWide(view, 4 + 0x0A, contextName);
            WriteWide(view, 4 + 0x312, appId);
            byte[] events = Encoding.ASCII.GetBytes(contextName + '\0');
            view.Write(0x712 + 4, 1u);
            view.WriteArray(0x712 + 0x210, events, 0, events.Length);
            view.Flush();
        }
    }

    public byte[] CreateAcknowledgement(ReadOnlySpan<byte> request)
    {
        if (request.Length != 796 || BinaryPrimitives.ReadUInt32LittleEndian(request) != 0x6B647331 ||
            BinaryPrimitives.ReadUInt32LittleEndian(request[4..]) != 1000)
            throw new InvalidDataException("Expected the validated SDK environment initialization request.");
        string abbreviation = Encoding.Unicode.GetString(request.Slice(18, 64)).Split('\0')[0];
        if (!Regex.IsMatch(abbreviation, @"\A[A-Za-z0-9_.-]{1,31}\z"))
            throw new InvalidDataException("Unsupported application abbreviation in the initialization request.");
        Dictionary<string, string> values = GetEnvironmentValues(abbreviation);
        byte[] payload =
            Encoding.ASCII.GetBytes(string.Join("|;|", values.Select(pair => pair.Key + "|=|" + pair.Value)));
        if (payload.Length >= 4096)
            throw new InvalidDataException("Environment synchronization exceeds its protocol buffer.");
        byte[] reply = new byte[0x1624];
        BinaryPrimitives.WriteUInt32LittleEndian(reply, 0x6B647331);
        BinaryPrimitives.WriteUInt32LittleEndian(reply.AsSpan(4), 1001);
        BinaryPrimitives.WriteUInt32LittleEndian(reply.AsSpan(8), (uint)reply.Length);
        payload.CopyTo(reply, 12);
        if (hostContext)
        {
            WriteWide(reply, 0x100C, platformExecutables[0]);
            WriteWide(reply, 0x1214, platformExecutables[1]);
            WriteWide(reply, 0x141C, platformExecutables[2]);
        }

        return reply;
    }

    public Dictionary<string, string> GetEnvironmentValues(string? abbreviation = null)
    {
        int titleChecksum = 0;
        foreach (char character in window.Title) titleChecksum ^= character;
        int key = startupContext ? 0 : int.Parse(appId, CultureInfo.InvariantCulture);
        Dictionary<string, string> values = new()
        {
            ["ArcProcessId"] = Environment.ProcessId.ToString(CultureInfo.InvariantCulture),
            ["ArcGameAppId"] = appId,
            ["ArcGameAppAbbr"] = abbreviation ?? appId,
            ["ArcMoblieNotify"] = window.Notification.ToString(CultureInfo.InvariantCulture),
            ["anchor_w"] = unchecked((int)window.Handle ^ key).ToString(CultureInfo.InvariantCulture),
            ["anchor_v"] = unchecked(1 - (titleChecksum ^ key)).ToString(CultureInfo.InvariantCulture),
            ["ArcMId"] = mapName,
            ["ArcMMId"] = mutexName
        };
        if (startupContext)
        {
            values["GamePlatformPath"] = launcher;
            values["wmgplaunch"] = "1";
            values["ArcGameMobileSdkType"] = "1";
            values["ArcGameMsgSkipFlag"] = "1";
            values["ArcDataIndex"] = appId; // record index zero XOR application ID
        }

        return values;
    }

    public object ContextEventState => new { supplied = contextEvent != null, signaled = contextEvent?.WaitOne(0) };

    private static void WriteWide(byte[] bytes, int offset, string value)
    {
        if (value.Length >= 260) throw new InvalidDataException("SDK context path is too long.");
        Encoding.Unicode.GetBytes(value + '\0').CopyTo(bytes, offset);
    }

    private static void WriteWide(MemoryMappedViewAccessor view, long offset, string value)
    {
        byte[] bytes = Encoding.Unicode.GetBytes(value + '\0');
        if (bytes.Length > 520) throw new InvalidDataException("SDK context field is too long.");
        view.WriteArray(offset, bytes, 0, bytes.Length);
    }

    public void Dispose()
    {
        contextEvent?.Dispose();
        readyEvent?.Dispose();
        mutex.Dispose();
        map.Dispose();
        window.Dispose();
    }
}
