using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;
using System.Text.Json;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;
using Avalonia.Automation;
using Avalonia.Platform;
using Avalonia.Win32;
using Avalonia.Controls.Primitives;
using Avalonia.Data;
using Avalonia.Media;
using Avalonia.Media.Imaging;
using Avalonia.Threading;

namespace Nte.LoginHost;

internal sealed partial class LoginWindow : Window
{
    private const double CardWidth = 368, CardHeight = 448, SceneWidth = 456, SceneHeight = 484;
    private readonly Canvas scene = new() { Width = SceneWidth, Height = SceneHeight, Background = null };
    private readonly TextBlock title = new() { FontSize = 20, FontWeight = FontWeight.SemiBold };
    private readonly TextBlock subtitle = new() { FontSize = 11, Opacity = .6, Margin = new(0, 7, 0, 0) };
    private readonly Grid content = new() { Margin = new(26, 24, 26, 20) };
    private readonly StackPanel form = new();
    private readonly TextBox phone = new() { Height = 40, MaxLength = 11 };
    private readonly TextBox captcha = new() { Height = 40, MaxLength = 8 };

    private readonly Button sms = new()
        { Content = Localization.LoginText.Get("SendSmsCode"), Height = 40, FontSize = 12 };

    private readonly Button login = new() { Content = Localization.LoginText.Get("SignIn"), Height = 42 };

    private readonly TextBlock status = new()
        { FontSize = 12, Opacity = .85, TextWrapping = TextWrapping.Wrap };

    private readonly StackPanel footer = new();
    private readonly Button closeLogin = new();
    private readonly Border card;
    private readonly Border materialTint = new() { CornerRadius = new(16) };

    private readonly Avalonia.Controls.Shapes.Rectangle cachedBackdrop = new()
        { RadiusX = 16, RadiusY = 16, IsHitTestVisible = false };

    private AcrylicBackdrop.Snapshot? backdrop;
    private bool backdropAttempted;
    private int backdropCaptureCount;
    private int backdropRefreshAttempts;
    private readonly BackdropRefreshSchedule backdropRefresh = new();
    private readonly DispatcherTimer backdropTimer = new();
    private readonly Stopwatch backdropClock = new();
    private readonly List<double> backdropCaptureTimes = [];
    private readonly GameCursor gameCursor = new();

    private readonly ImageBrush backdropBrush = new() { Stretch = Stretch.Fill };

    private readonly ScrollViewer scroll;

    private readonly Image mascot;
    private readonly Dictionary<string, Bitmap> mascotImages = new(StringComparer.Ordinal);

    // Discover windows and retry pending presentation; visible-window geometry
    // continues to follow SDK notifications and WinEvents.
    private readonly DispatcherTimer tracker = new() { Interval = TimeSpan.FromSeconds(1) };
    private readonly nint handle;
    private readonly Win32Properties.CustomWindowStylesCallback windowStyles;
    private readonly Viewbox view;
    private readonly WindowNative.WinEventProcedure windowEvents;
    private readonly List<nint> eventHooks = [];
    private bool layoutQueued, raiseQueued, raisedForGame;
    private int sdkGeometryNotifications, windowGeometryNotifications, sdkLoginNotifications;
    private int placementChanges, stackingChanges;
    private Process? game;
    private nint gameWindow, avaloniaOwner;
    private bool requested, preview, busy, stopping, helpOpen;
    private bool syntheticLayoutOnly;
    private string currentTheme = "";
    private int attempt;
    private CancellationTokenSource? operation;
    private ProbeAuthenticationClient? client;
    private Func<ProbeIdentityEnvelope, ProbeIdentityEnvelope, CancellationToken, Task>? deliver;
    private TaskCompletionSource<bool>? completion;
    private TaskCompletionSource? failureCompletion;
    private WelcomeToast? welcome;
    private bool automaticResumeInProgress;
    private bool cardSizeQueued;
    private double maximumNoticeHeight = 610;

    private void ShowWelcome(string label)
    {
        welcome ??= new WelcomeToast();
        welcome.Present(label, gameWindow, preview ? TimeSpan.FromSeconds(30) : TimeSpan.FromSeconds(4));
    }

    internal LoginWindow(bool exposePreview = false)
    {
        Title = Localization.LoginText.Get("MainlandTitle");
        SystemDecorations = SystemDecorations.None;
        CanResize = false;
        TransparencyLevelHint = [WindowTransparencyLevel.Transparent];
        Background = Brushes.Transparent;
        ShowInTaskbar = exposePreview;
        ShowActivated = false;
        FontFamily = new("Microsoft YaHei UI");
        UseLayoutRounding = true;
        this.Bind(ForegroundProperty, this.GetResourceObservable("TextBrush"));
        content.RowDefinitions.Add(new() { Height = GridLength.Auto });
        content.RowDefinitions.Add(new() { Height = GridLength.Auto });
        content.RowDefinitions.Add(new() { Height = new(1, GridUnitType.Star) });
        content.RowDefinitions.Add(new() { Height = GridLength.Auto });
        StackPanel heading = new() { Margin = new(0, 0, 0, 16) };
        heading.Children.Add(title);
        heading.Children.Add(subtitle);
        content.Children.Add(heading);
        Button cancel = closeLogin;
        cancel.Content = "×";
        cancel.Width = cancel.Height = 24;
        cancel.HorizontalAlignment = HorizontalAlignment.Right;
        cancel.VerticalAlignment = VerticalAlignment.Top;
        cancel.FontSize = 17;
        cancel.Theme = FindStyle("QuietButtonStyle");
        AutomationProperties.SetName(cancel, Localization.LoginText.Get("CancelSignIn"));
        cancel.Click += (_, _) =>
        {
            if (MandatoryNotice || noticeConfirmation) return;
            if (helpOpen)
            {
                ReturnFromNotice();
                return;
            }

            if (failureCompletion != null) failureCompletion.TrySetResult();
            else CancelFlow();
        };
        content.Children.Add(cancel);
        Grid.SetRow(modeRail, 1);
        content.Children.Add(modeRail);
        scroll = new()
        {
            Content = form, VerticalScrollBarVisibility = ScrollBarVisibility.Auto,
            HorizontalScrollBarVisibility = ScrollBarVisibility.Disabled,
            HorizontalContentAlignment = HorizontalAlignment.Stretch, Theme = FindStyle("LoginScrollViewerStyle")
        };
        Grid.SetRow(scroll, 2);
        content.Children.Add(scroll);
        footer.Margin = new(0, 16, 0, 0);
        status.Margin = new(0, 0, 0, 10);
        footer.Children.Add(status);
        footer.Children.Add(login);
        Grid.SetRow(footer, 3);
        content.Children.Add(footer);
        // Keep the shadow's intermediate bitmap separate from the live text.
        Grid material = new()
        {
            IsHitTestVisible = false,
            Effect = new DropShadowEffect { BlurRadius = 22, OffsetY = 4, Color = Color.FromArgb(56, 0, 0, 0) }
        };
        material.Children.Add(cachedBackdrop);
        material.Children.Add(materialTint);
        Grid layers = new();
        layers.Children.Add(material);
        layers.Children.Add(content);
        card = new()
        {
            Width = CardWidth, Height = CardHeight, CornerRadius = new(17), BorderThickness = new(1), Child = layers
        };
        Canvas.SetLeft(card, 10);
        Canvas.SetTop(card, 18);
        scene.Children.Add(card);
        mascot = new()
        {
            Width = 72, Height = 92, Stretch = Stretch.Uniform
        };
        mascot.Source = GetMascot("day");
        RenderOptions.SetBitmapInterpolationMode(mascot, BitmapInterpolationMode.HighQuality);
        Button mascotButton = new()
        {
            Content = mascot, Width = 76, Height = 96, Padding = new(0), Theme = FindStyle("QuietButtonStyle"),
            FontSize = 12
        };
        AutomationProperties.SetName(mascotButton, Localization.LoginText.Get("ReadSignInHelp"));
        AutomationProperties.SetAutomationId(mascotButton, "help");
        mascotButton.Click += (_, _) => ToggleHelp();
        Canvas.SetLeft(mascotButton, 362);
        Canvas.SetTop(mascotButton, 44);
        scene.Children.Add(mascotButton);
        view = new Viewbox
        {
            Child = scene, Stretch = Stretch.Uniform,
            HorizontalAlignment = HorizontalAlignment.Right, VerticalAlignment = VerticalAlignment.Center,
            Margin = new(18)
        };
        view.Bind(Avalonia.Controls.Documents.TextElement.ForegroundProperty, this.GetResourceObservable("TextBrush"));
        view.SetValue(Avalonia.Controls.Documents.TextElement.FontFamilyProperty, FontFamily);
        Grid cover = new() { Background = new SolidColorBrush(Color.FromArgb(8, 0, 0, 0)) };
        cover.Children.Add(view);
        Content = cover;
        phone.Theme = FindStyle("LoginTextBoxStyle");
        captcha.Theme = FindStyle("LoginTextBoxStyle");
        login.Theme = FindStyle("AccentButtonStyle");
        sms.Theme = FindStyle("SmsButtonStyle");
        phone.Height = captcha.Height = 42;
        phone.Padding = new(39, 9, 12, 9);
        captcha.Padding = new(39, 9, 12, 9);
        sms.Height = 42;
        sms.MinWidth = 94;
        sms.MaxWidth = 150;
        sms.HorizontalAlignment = HorizontalAlignment.Right;
        sms.VerticalAlignment = VerticalAlignment.Center;
        sms.Margin = new(8, 0, 0, 0);
        sms.Padding = new(6, 3, 6, 3);
        ConfigureSmsCooldown();
        login.Click += async (_, _) =>
        {
            if (failureCompletion != null) failureCompletion.TrySetResult();
            else if (helpOpen) AcknowledgeNotice();
            else if (!initialized && !preview) await InitializeLoginAsync();
            else if (localMode && selectedAccount == null) await SwitchModeAsync(false);
            else await ExecuteAsync(LoginWithSelectedAccountAsync);
        };
        tracker.Tick += (_, _) => DiscoverAndUpdateTheme();
        backdropTimer.Tick += (_, _) => RefreshBackdrop();
        windowEvents = (_, eventType, window, objectId, childId, _, _) =>
        {
            if (eventType != 3 && (objectId != 0 || childId != 0)) return;
            if (eventType == 3 || window == gameWindow || !WindowNative.IsWindow(gameWindow))
            {
                Interlocked.Increment(ref windowGeometryNotifications);
                QueueLayout();
            }
        };
        Closing += (_, e) =>
        {
            if (!stopping)
            {
                e.Cancel = true;
                if (MandatoryNotice || noticeConfirmation) return;
                if (failureCompletion != null) failureCompletion.TrySetResult();
                else CancelFlow();
            }
        };
        handle = TryGetPlatformHandle()?.Handle ??
                 throw new InvalidOperationException("Native login window unavailable.");
        // Avalonia reapplies its styles on Show, including after an Alt+Tab.
        windowStyles = (style, extended) => ((style | 0x80000000u) & ~0x40000000u,
            exposePreview ? (extended | 0x40000u) & ~0x80u : (extended | 0x80u) & ~0x40000u);
        Win32Properties.AddWindowStylesCallback(this, windowStyles);
        Win32Properties.AddWndProcHookCallback(this, NativeMessage);
        Opened += (_, _) =>
        {
            // Complete layout after the native surface has opened, even if the
            // game stays still and sends no subsequent location-change event.
            QueueBodyWidth();
            QueueCardSize();
            QueueLayout();
        };
        ConfigureAccounts();
        view.SizeChanged += (_, _) => UpdateBackdropViewbox();
        scroll.SizeChanged += (_, _) => QueueBodyWidth();
        form.SizeChanged += (_, _) =>
        {
            QueueBodyWidth();
            QueueCardSize();
        };
        status.PropertyChanged += (_, args) =>
        {
            if (args.Property != TextBlock.TextProperty) return;
            status.IsVisible = !string.IsNullOrWhiteSpace(status.Text) && globalView == null;
            QueueCardSize();
        };
        KeyDown += (_, args) =>
        {
            if (args.Key == Avalonia.Input.Key.Escape && helpOpen)
            {
                if (noticeConfirmation) ShowLoginNotice();
                else if (!MandatoryNotice) ReturnFromNotice();
                args.Handled = true;
            }
        };
        officialLauncher.Theme = FindStyle("SmsButtonStyle");
        officialLauncher.Margin = new(0, 8, 0, 0);
        officialLauncher.MinHeight = 36;
        officialLauncher.IsVisible = false;
        AutomationProperties.SetAutomationId(officialLauncher, "official-launcher");
        officialLauncher.Click += (_, _) => ChooseOfficialLauncher();
        footer.Children.Add(officialLauncher);
        ShowForm();
        ApplyClockTheme();
    }

    private ControlTheme FindStyle(string name) => (ControlTheme)this.FindResource(name)!;

    private static void Add(StackPanel parent, Control element, double bottom = 14)
    {
        element.Margin = new(0, 0, 0, bottom);
        parent.Children.Add(element);
    }

    private static StackPanel Group(string title, Control field)
    {
        StackPanel group = new();
        group.Children.Add(new TextBlock { Text = title, FontSize = 12, Opacity = .85, Margin = new(0, 0, 0, 8) });
        group.Children.Add(field);
        return group;
    }

    private Grid PhoneField()
    {
        Grid field = new();
        field.Children.Add(phone);
        TextBlock hint = new()
        {
            Text = Localization.LoginText.Get("PhonePlaceholder"), FontSize = 13, Opacity = .42,
            IsHitTestVisible = false, VerticalAlignment = VerticalAlignment.Center, Margin = new(39, 0, 12, 0)
        };
        hint.IsVisible = (phone.Text?.Length ?? 0) == 0;
        phone.TextChanged += (_, _) =>
            hint.IsVisible = (phone.Text?.Length ?? 0) == 0;
        field.Children.Add(hint);
        field.Children.Add(FieldIcon("\uE8EA"));
        return field;
    }

    private Grid CodeField()
    {
        Grid field = new();
        field.ColumnDefinitions.Add(new());
        field.ColumnDefinitions.Add(new() { Width = GridLength.Auto });
        field.Children.Add(captcha);
        TextBlock hint = new()
        {
            Text = Localization.LoginText.Get("CodePlaceholder"), FontSize = 13, Opacity = .42,
            IsHitTestVisible = false, VerticalAlignment = VerticalAlignment.Center, Margin = new(39, 0, 12, 0),
            TextTrimming = TextTrimming.CharacterEllipsis
        };
        hint.IsVisible = (captcha.Text?.Length ?? 0) == 0;
        captcha.TextChanged += (_, _) =>
            hint.IsVisible = (captcha.Text?.Length ?? 0) == 0;
        field.Children.Add(hint);
        field.Children.Add(FieldIcon("\uE72E"));
        Grid.SetColumn(sms, 1);
        field.Children.Add(sms);
        return field;
    }

    private static TextBlock FieldIcon(string glyph) => new()
    {
        Text = glyph, FontFamily = new("Segoe Fluent Icons, Segoe MDL2 Assets"), FontSize = 16, Width = 18,
        Opacity = .65, IsHitTestVisible = false, VerticalAlignment = VerticalAlignment.Center,
        HorizontalAlignment = HorizontalAlignment.Left,
        TextAlignment = TextAlignment.Center, Margin = new(13, 0, 0, 0)
    };

    private void QueueBodyWidth() => Dispatcher.UIThread.Post(() =>
    {
        if (scroll.Viewport.Width > 0 &&
            (double.IsNaN(form.Width) || Math.Abs(form.Width - scroll.Viewport.Width) > .1))
            form.Width = scroll.Viewport.Width;
    }, DispatcherPriority.Loaded);

    private Grid? phoneField, codeField;
    private StackPanel? phoneGroup, verificationGroup;

    private void UpdateCardSize()
    {
        // Measure at the actual inner card width with no scrollbar. A scrollbar
        // from the previous page must not steal width and create another wrap.
        form.Width = double.NaN;
        scroll.VerticalScrollBarVisibility = ScrollBarVisibility.Disabled;
        content.Measure(new Size(CardWidth - card.BorderThickness.Left - card.BorderThickness.Right,
            double.PositiveInfinity));
        // Avalonia's ScrollViewer retains a viewport-constrained desired size.
        // Measure the body itself so changing pages can grow the card instead
        // of clipping the last input/checkbox to the previous page's height.
        double innerWidth = CardWidth - card.BorderThickness.Left - card.BorderThickness.Right - content.Margin.Left -
                            content.Margin.Right;
        Size available = new(innerWidth, double.PositiveInfinity);
        form.Measure(available);
        footer.Measure(available);
        modeRail.Measure(available);
        Control heading = content.Children[0];
        heading.Measure(available);
        double naturalHeight = Math.Ceiling(heading.DesiredSize.Height + modeRail.DesiredSize.Height +
                                            form.DesiredSize.Height +
                                            footer.DesiredSize.Height + content.Margin.Top + content.Margin.Bottom +
                                            card.BorderThickness.Top + card.BorderThickness.Bottom + 4);
        double height = Math.Clamp(naturalHeight, 260, helpOpen ? maximumNoticeHeight : 610);
        scroll.VerticalScrollBarVisibility =
            naturalHeight > height ? ScrollBarVisibility.Auto : ScrollBarVisibility.Disabled;
        if (Math.Abs(card.Height - height) < .5) return;
        card.Height = height;
        scene.Height = height + SceneHeight - CardHeight;
        if (requested) QueueLayout();
    }

    private void QueueCardSize()
    {
        if (cardSizeQueued || stopping) return;
        cardSizeQueued = true;
        Dispatcher.UIThread.Post(() =>
        {
            cardSizeQueued = false;
            if (!stopping) UpdateCardSize();
        }, DispatcherPriority.Loaded);
    }

    private void ShowForm()
    {
        form.Children.Clear();
        helpOpen = false;
        title.Text = Localization.LoginText.Get("MainlandTitle");
        subtitle.Text = "";
        subtitle.IsVisible = false;
        footer.IsVisible = true;
        modeRail.IsVisible = false;
        status.IsVisible = !(string.IsNullOrWhiteSpace(status.Text));
        AddAccountTabs();
        if (localMode)
        {
            BuildAccounts();
            RefreshMainlandNavigation();
            UpdateCardSize();
            QueueBodyWidth();
            scroll.ScrollToHome();
            return;
        }

        phoneField ??= PhoneField();
        codeField ??= CodeField();
        phoneGroup ??= Group(Localization.LoginText.Get("Phone"), phoneField);
        verificationGroup ??= Group(Localization.LoginText.Get("SmsCode"), codeField);
        Add(form, phoneGroup, 18);
        Add(form, verificationGroup, 14);
        AddAccountSaveFields();
        login.Content = !initialized && !preview
            ? Localization.LoginText.Get("RetryConnection")
            : Localization.LoginText.Get("SignIn");
        login.IsEnabled = !busy && (preview || initialized || requested);
        RefreshMainlandNavigation();
        UpdateCardSize();
        QueueBodyWidth();
        scroll.ScrollToHome();
    }

    private void ToggleHelp()
    {
        if (MandatoryNotice || noticeConfirmation) return;
        if (helpOpen)
        {
            ReturnFromNotice();
            return;
        }

        noticeGlobal = globalView != null;
        ShowLoginNotice();
    }

    internal async Task<bool> RequestAsync(ProbeAuthenticationClient api,
        Func<ProbeIdentityEnvelope, ProbeIdentityEnvelope, CancellationToken, Task> submit, CancellationToken token,
        Func<CancellationToken, Task>? reset = null)
    {
        CancelFlow();
        await activeWork;
        client = api;
        deliver = submit;
        resetAdmission = reset;
        requestToken = token;
        operation?.Dispose();
        operation = CancellationTokenSource.CreateLinkedTokenSource(token);
        completion = new(TaskCreationOptions.RunContinuationsAsynchronously);
        requested = true;
        preview = false;
        Nte.Login.NteHostStartupDiagnostics.Phase("loading-saved-account-displays");
        IReadOnlyList<Nte.LoginHost.Accounts.LocalAccount> savedAccounts = accounts.List();
        await LoadMainlandDisplaysAsync(token);
        localMode = savedAccounts.Count > 0;
        initialized = false;
        bool resumeLast = false;
        int resumeGeneration = attempt;
        Guid? resumeAccount = null;
        try
        {
            Nte.Login.NteHostStartupDiagnostics.Phase("loading-remembered-selection");
            await rememberedChange;
            Guid? last = await remembered.ReadAsync(token);
            selectedAccount = Nte.LoginHost.Accounts.RememberedLogin.Resolve(last, remembered.HasState, savedAccounts);
            resumeLast = selectedAccount != null && !autoResumeDisabled;
            resumeAccount = selectedAccount?.Id;
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException or InvalidDataException
                                       or CryptographicException)
        {
            autoResumeDisabled = true;
        }

        ResetBackdrop();
        using CancellationTokenRegistration registration =
            token.Register(() => Dispatcher.UIThread.Post(CancelForGameExit));
        automaticResumeInProgress = resumeLast;
        if (resumeLast)
        {
            welcome ??= new WelcomeToast();
            welcome.PresentMessage(Localization.LoginText.Get("RestoringLastAccount"), gameWindow,
                TimeSpan.FromSeconds(60));
        }

        ShowForm();
        Nte.Login.NteHostStartupDiagnostics.Phase("presenting-login-panel");
        Track(raise: true);
        try
        {
            Nte.Login.NteHostStartupDiagnostics.Phase("login-panel-initializing");
            await InitializeLoginAsync();
            Nte.Login.NteHostStartupDiagnostics.Phase("login-panel-ready");
            if (resumeLast && initialized && requested && attempt == resumeGeneration && localMode && !helpOpen &&
                selectedAccount?.Id == resumeAccount)
            {
                automaticAttempt = true;
                await ExecuteAsync(LoginWithSelectedAccountAsync);
            }
        }
        finally
        {
            automaticAttempt = automaticResumeInProgress = false;
            if (requested) welcome?.Dismiss();
            Track(raise: resumeLast);
        }

        return await completion.Task;
    }

    private Task InitializeLoginAsync() => ExecuteAsync(async (active, cancel, generation) =>
    {
        status.Text = Localization.LoginText.Get("InitializingSignIn");
        experimentalHello = ReadProtectionPreference();
        await active.InitializeAsync(cancel);
        RequireCurrent(generation, cancel);
        initialized = true;
        status.Text = "";
        ShowForm();
    }, true);

    internal void Attach(int pid)
    {
        foreach (nint hook in eventHooks) WindowNative.UnhookWinEvent(hook);
        eventHooks.Clear();
        game?.Dispose();
        game = Process.GetProcessById(pid);
        foreach ((uint low, uint high, uint process) in new[]
                     { (0x8000u, 0x800Bu, (uint)pid), (0xAu, 0x17u, (uint)pid), (3u, 3u, 0u) })
        {
            nint hook = WindowNative.SetWinEventHook(low, high, 0, windowEvents, process, 0, 2);
            if (hook == 0) throw new System.ComponentModel.Win32Exception();
            eventHooks.Add(hook);
        }

        tracker.Start();
    }

    internal async Task PreviewAsync(int pid, bool welcomeOnly, bool waitForSdkRequest)
    {
        Attach(pid);
        preview = initialized = true;
        Task gameExit = game!.WaitForExitAsync();
        using ProbeWindow? bridge = waitForSdkRequest ? new() : null;
        if (bridge != null && !await WaitForPreviewLoginAsync(bridge, gameExit)) return;
        requested = true;
        SetMainlandPreviewAccounts(3);
        ResetBackdrop();
        status.Text = "";
        SetAccountControlsEnabled(true);
        ShowForm();
        Track(raise: true);
        if (welcomeOnly)
        {
            requested = false;
            HidePresented();
            ShowWelcome(Localization.LoginText.Get("PreviewMainAccount"));
        }

        await gameExit;
    }

    private Task ExecuteAsync(Func<ProbeAuthenticationClient, CancellationToken, int, Task> action,
        bool initialize = false)
    {
        if (busy) return Task.CompletedTask;
        if (preview)
        {
            status.Text = Localization.LoginText.Get("PreviewNoAuthentication");
            return Task.CompletedTask;
        }

        if (client == null || operation == null || (!initialize && !initialized)) return Task.CompletedTask;
        activeWork = RunActionAsync(action, client, operation.Token, attempt);
        return activeWork;
    }

    private async Task RunActionAsync(Func<ProbeAuthenticationClient, CancellationToken, int, Task> action,
        ProbeAuthenticationClient api, CancellationToken token, int generation)
    {
        busy = true;
        SetAccountControlsEnabled(false);
        try
        {
            await action(api, token, generation);
        }
        catch (ProbeAuthenticationException ex)
        {
            if (generation == attempt)
            {
                if (ex.Phase == "channel-validation-rejected")
                {
                    localMode = false;
                    phone.Clear();
                    captcha.Clear();
                    ShowForm();
                }

                status.Text = ex.LocalExplanation ?? Friendly(ex.Phase);
            }
        }
        catch (OperationCanceledException)
        {
            if (generation == attempt) status.Text = Localization.LoginText.Get("OperationCancelled");
        }
        catch (Nte.LoginHost.Accounts.HelloProtectionException ex)
        {
            if (generation == attempt)
                status.Text = Localization.LoginText.Get("HelloProtectionFailed", $"0x{ex.NativeError:X8}");
        }
        catch (System.Security.Cryptography.CryptographicException)
        {
            if (generation == attempt) status.Text = Localization.LoginText.Get("AccountUnlockFailed");
        }
        catch (InvalidDataException)
        {
            if (generation == attempt) status.Text = Localization.LoginText.Get("SavedAccountUnavailable");
        }
        catch (Exception)
        {
            if (generation == attempt) status.Text = Localization.LoginText.Get("SignInIncomplete");
        }
        finally
        {
            if (generation == attempt)
            {
                busy = false;
                SetAccountControlsEnabled(initialized);
                if (!initialized && !preview && requested && !helpOpen)
                {
                    login.Content = Localization.LoginText.Get("RetryConnection");
                    login.IsEnabled = true;
                }
            }
        }
    }

    private void CancelFlow()
    {
        noticeCompletion?.TrySetResult(false);
        welcome?.Dismiss();
        automaticResumeInProgress = false;
        attempt++;
        requested = false;
        operation?.Cancel();
        completion?.TrySetResult(false);
        busy = false;
        phone.Clear();
        captcha.Clear();
        globalView?.Cancel();
        HidePresented();
        ResetBackdrop();
        UpdateSmsButton();
    }

    internal void CancelForGameExit() => CancelFlow();

    internal void ReportServiceFailure(string text) => Dispatcher.UIThread.Post(() =>
    {
        if (globalView != null) globalView.ShowNotice(text);
        else status.Text = text;
    });

    internal void NotifySdkGeometryChanged()
    {
        Interlocked.Increment(ref sdkGeometryNotifications);
        QueueLayout();
    }

    internal void NotifySdkLoginRequested()
    {
        Interlocked.Increment(ref sdkLoginNotifications);
        QueueLayout(raise: true);
    }

    private void QueueLayout(bool raise = false)
    {
        if (!Dispatcher.UIThread.CheckAccess())
        {
            Dispatcher.UIThread.Post(() => QueueLayout(raise), DispatcherPriority.Send);
            return;
        }

        if (stopping) return;
        raiseQueued |= raise;
        if (layoutQueued) return;
        layoutQueued = true;
        Dispatcher.UIThread.Post(() =>
        {
            layoutQueued = false;
            bool raiseNow = raiseQueued;
            raiseQueued = false;
            Track(raiseNow);
        }, DispatcherPriority.Send);
    }

    private nint NativeMessage(nint window, uint message, nint wParam, nint lParam, ref bool handled)
    {
        if (message is 0x2E0 or 0x7E) QueueLayout();
        if (message == 0x20 && ((int)lParam & 0xffff) == 1 && IsVisible && requested &&
            gameCursor.Synchronize(handle))
        {
            handled = true;
            return 1;
        }

        return 0;
    }

    private void DiscoverAndUpdateTheme()
    {
        ApplyClockTheme();
        if (welcome?.Active == true && game != null && !game.HasExited)
            gameWindow = WindowNative.FindGameWindow(game.Id);
        welcome?.Track(gameWindow);
        if (game == null || game.HasExited || !requested)
        {
            HidePresented();
            return;
        }

        nint target = WindowNative.FindGameWindow(game.Id);
        if (target != gameWindow || !IsVisible || !WindowNative.IsWindowVisible(handle) ||
            (helloPrompts == 0 && (!HasGameForeground(game.Id) ||
                                   !WindowNative.IsTopmost(handle) || !WindowNative.IsWindowAbove(handle, target))))
        {
            QueueLayout();
        }

        if (preview) _ = ProcessPreviewActionAsync();
        if (preview) WritePreviewState();
    }

    private bool HasGameForeground(int gamePid)
    {
        if (preview && !previewFollowForeground) return true;
        WindowNative.GetWindowThreadProcessId(WindowNative.GetForegroundWindow(), out uint foregroundPid);
        return foregroundPid == gamePid || foregroundPid == Environment.ProcessId;
    }

    private void Track(bool raise = false)
    {
        if (syntheticLayoutOnly) return;
        if (failureCompletion != null) return;
        ApplyClockTheme();
        if (welcome?.Active == true && game != null && !game.HasExited)
            gameWindow = WindowNative.FindGameWindow(game.Id);
        welcome?.Track(gameWindow);
        if (automaticResumeInProgress)
        {
            HidePresented();
            return;
        }

        if (game == null || game.HasExited || !requested)
        {
            HidePresented();
            return;
        }

        nint target = WindowNative.FindGameWindow(game.Id);
        if (target == 0 || WindowNative.IsIconic(target))
        {
            HidePresented();
            return;
        }

        if (gameWindow != target) gameCursor.Reset();
        gameWindow = target;
        // A native Hello dialog runs in a system process. Keep its owner alive
        // and visible during that operation instead of hiding it on activation.
        if (helloPrompts == 0 && !HasGameForeground(game.Id))
        {
            HidePresented();
            return;
        }

        WindowNative.Rect clientRect = WindowNative.ScreenClient(target);
        if (clientRect.Width < 320 || clientRect.Height < 180)
        {
            HidePresented();
            return;
        }

        gameCursor.ObserveGame(target);
        if (!backdropAttempted && !IsVisible)
        {
            backdropAttempted = true;
            backdropClock.Restart();
            backdropRefresh.Start(TimeSpan.Zero);
            if (!NativeUiSettings.HighContrast)
            {
                backdrop = AcrylicBackdrop.Capture(target, clientRect);
                if (backdrop != null)
                {
                    backdropCaptureCount++;
                    backdropCaptureTimes.Add(backdropClock.Elapsed.TotalSeconds);
                    backdropBrush.Source = backdrop.Image;
                    cachedBackdrop.Fill = backdropBrush;
                }
            }

            UpdateMaterialTint();
            ScheduleBackdropRefresh();
        }

        bool opening = !IsVisible;
        if (opening)
        {
            // Give Avalonia the actual client size before its first layout.
            // Updating only the HWND after Show can leave the first frame with
            // the hidden window's default size until the owner moves.
            Position = new PixelPoint(clientRect.Left, clientRect.Top);
            double initialScale = Math.Max(1, WindowNative.GetDpiForWindow(handle) / 96d);
            ClientSize = new Size(clientRect.Width / initialScale, clientRect.Height / initialScale);
        }

        double scale = Math.Max(1, WindowNative.GetDpiForWindow(handle) / 96d);
        // Long notices should scroll at their normal font size before the view
        // resorts to scaling the entire card to fit the game's client area.
        double availableCardHeight = Math.Clamp(clientRect.Height / scale - view.Margin.Top - view.Margin.Bottom -
                                                (SceneHeight - CardHeight), 260, 610);
        if (Math.Abs(maximumNoticeHeight - availableCardHeight) > .1)
        {
            maximumNoticeHeight = availableCardHeight;
            if (helpOpen) UpdateCardSize();
        }

        double availableWidth = clientRect.Width / scale - view.Margin.Left - view.Margin.Right;
        double width = Math.Min(SceneWidth, helpOpen ? availableWidth : availableWidth * .62);
        double height = Math.Min(scene.Height, clientRect.Height / scale - 28);
        width = Math.Min(width, height * SceneWidth / scene.Height);
        view.Width = width;
        view.Height = width * scene.Height / SceneWidth;
        if (opening)
        {
            Show();
            avaloniaOwner = WindowNative.GetWindow(handle, 4);
            UpdateLayout();
        }

        if (helloPrompts == 0 && !HasGameForeground(game.Id))
        {
            HidePresented();
            return;
        }

        // Avalonia Show resets the native owner to its own hidden helper.
        // Restore the external owner afterwards, before repairing the Z order.
        WindowNative.SetOwner(handle, target);

        // Show may apply the platform's initial sizing. Compare after it, and
        // also recover a native surface hidden independently of IsVisible.
        WindowNative.Rect previous = WindowNative.ScreenClient(handle);
        bool nativeHidden = !WindowNative.IsWindowVisible(handle);
        bool raiseForGame = helloPrompts == 0 && (raise || opening || nativeHidden ||
            !WindowNative.IsTopmost(handle) || !WindowNative.IsWindowAbove(handle, target));
        if (nativeHidden || raiseForGame || previous.Left != clientRect.Left || previous.Top != clientRect.Top ||
            previous.Width != clientRect.Width || previous.Height != clientRect.Height)
        {
            // Keep the owned panel above the game even when the game raises
            // itself. The foreground guard bounds this promotion to the login
            // session, and SWP_NOACTIVATE leaves the user's focus alone.
            uint flags = 0x210u | (raiseForGame ? 0u : 0x4u) | (nativeHidden ? 0x40u : 0u);
            nint insertAfter = raiseForGame ? -1 : 0;
            if (!WindowNative.SetWindowPos(handle, insertAfter, clientRect.Left, clientRect.Top, clientRect.Width,
                    clientRect.Height, flags))
                throw new System.ComponentModel.Win32Exception();
            placementChanges++;
            if (raiseForGame)
            {
                raisedForGame = true;
                stackingChanges++;
            }
        }
        UpdateBackdropViewbox();
        if (opening || raiseForGame) gameCursor.Synchronize(handle);
        if (preview) WritePreviewState();
    }

    private void ApplyClockTheme() => ApplyTheme(LoginTheme.ForLocalTime(DateTime.Now));

    private Bitmap GetMascot(string theme)
    {
        string asset = LoginTheme.MascotAsset(theme);
        if (!mascotImages.TryGetValue(asset, out Bitmap? image))
        {
            image = new Bitmap(Path.Combine(AppContext.BaseDirectory, "Assets", asset));
            mascotImages.Add(asset, image);
        }
        return image;
    }

    private void ApplyTheme(string theme)
    {
        if (theme == currentTheme) return;
        mascot.Source = GetMascot(theme);
        currentTheme = theme;
        bool dark = theme == "night", warm = theme == "sunset";
        Resources["TextBrush"] = new SolidColorBrush(dark ? Color.FromRgb(232, 241, 247) : Color.FromRgb(31, 45, 53));
        Resources["AccentBrush"] = new SolidColorBrush(warm ? Color.FromRgb(154, 105, 47) :
            dark ? Color.FromRgb(39, 154, 175) : Color.FromRgb(28, 123, 143));
        Resources["InputBrush"] =
            new SolidColorBrush(dark ? Color.FromArgb(22, 218, 234, 246) : Color.FromArgb(178, 255, 255, 255));
        Resources["InputBorderBrush"] =
            new SolidColorBrush(dark ? Color.FromArgb(25, 232, 243, 250) : Color.FromArgb(25, 55, 90, 105));
        Resources["GlobalPopupBrush"] =
            new SolidColorBrush(dark ? Color.FromRgb(31, 45, 64) : Color.FromRgb(246, 250, 252));
        UpdateMaterialTint();
        card.BorderBrush =
            new SolidColorBrush(dark ? Color.FromArgb(35, 222, 237, 248) : Color.FromArgb(40, 100, 123, 134));
    }

    private void WritePreviewState()
    {
        if (syntheticLayoutOnly) return;
        var previewPixels = PreviewBackdropProbe();
        WindowNative.Rect bounds = WindowNative.ScreenClient(handle);
        string path = Path.Combine(AppContext.BaseDirectory, "preview-layout.json"), temporary = path + ".tmp";
        File.WriteAllText(temporary, JsonSerializer.Serialize(new Dictionary<string, object?>
        {
            ["theme"] = currentTheme,
            ["themeSource"] = "system-local-time",
            ["themeDerivedFromPixels"] = false,
            ["visible"] = IsVisible && WindowNative.IsWindowVisible(handle),
            ["nativeTopmost"] = WindowNative.IsTopmost(handle),
            ["mascotAsset"] = LoginTheme.MascotAsset(currentTheme),
            ["gamePid"] = game?.Id,
            ["hostPid"] = Environment.ProcessId,
            ["width"] = bounds.Width,
            ["height"] = bounds.Height,
            ["xamlWidth"] = card.Bounds.Width,
            ["xamlHeight"] = card.Bounds.Height,
            ["rasterScale"] = RenderScaling,
            ["formDesiredHeight"] = form.DesiredSize.Height,
            ["formAvailableHeight"] = scroll.Viewport.Height,
            ["formWidth"] = form.Bounds.Width,
            ["viewportWidth"] = scroll.Viewport.Width,
            ["footerWidth"] = footer.Bounds.Width,
            ["scrollable"] = scroll.Extent.Height > scroll.Viewport.Height,
            ["slider"] = true,
            ["fieldIcons"] = 2,
            ["duplicateHelpLink"] = false,
            ["modernSaveToggle"] = true,
            ["saveEnabled"] = saveAccount.IsChecked == true,
            ["labelEditorOnMain"] = false,
            ["experimentalHello"] = experimentalHello,
            ["smsCooldown"] = new Dictionary<string, object?>
            {
                ["enabled"] = sms.IsEnabled,
                ["remainingSeconds"] = smsRemainingSeconds,
                ["text"] = sms.Content?.ToString(),
                ["normalTextColor"] = Equals(sms.Foreground, Resources["TextBrush"]),
                ["surfaceOpacity"] = (double?)null
            },
            ["savedAccounts"] = new Dictionary<string, object?>
            {
                ["count"] = previewAccounts.Count,
                ["ids"] = previewAccounts.Select(a => a.Id).ToArray(),
                ["selected"] = selectedAccount?.Id
            },
            ["mascotReady"] = mascot.Source != null,
            ["mascotMirrored"] = (mascot.RenderTransform?.Value.M11 ?? 1) < 0,
            ["segmentFocusOutline"] = false,
            ["gameCursorSynchronized"] = gameCursor.HasCursor,
            ["gameCursorHandle"] = gameCursor.ObservedHandle,
            ["cursorSynchronizations"] = gameCursor.Synchronizations,
            ["renderer"] = "Avalonia-NativeAOT-owned-client-cover",
            ["nativeWindowsForLogin"] = 1,
            ["nativeChild"] = false,
            ["ownerWindow"] = (long)WindowNative.GetWindow(handle, 4),
            ["gameWindow"] = (long)gameWindow,
            ["aboveGame"] = WindowNative.IsWindowAbove(handle, gameWindow),
            ["stackingChanges"] = stackingChanges,
            ["fullClientCover"] = true,
            ["geometryPolling"] = false,
            ["placementChanges"] = placementChanges,
            ["sdkGeometryNotifications"] = sdkGeometryNotifications,
            ["sdkLoginNotifications"] = sdkLoginNotifications,
            ["windowGeometryNotifications"] = windowGeometryNotifications,
            ["accountRequests"] = 0,
            ["helloInvoked"] = false,
            ["acrylic"] = new Dictionary<string, object?>
            {
                ["cached"] = backdrop != null,
                ["captureCount"] = backdropCaptureCount,
                ["captureAttempted"] = backdropAttempted,
                ["refreshAttempts"] = backdropRefreshAttempts,
                ["refreshAtSeconds"] = new[] { 5, 15, 60 },
                ["captureTimesSeconds"] = backdropCaptureTimes.ToArray(),
                ["preparationMs"] = backdrop?.PreparationMs,
                ["cacheBytes"] = backdrop?.Bytes ?? 0,
                ["previewSampleSpread"] = previewPixels.Spread,
                ["previewSamples"] = previewPixels.Samples,
                ["continuousCapture"] = false,
                ["savedToDisk"] = false
            }
        }, NteJsonContext.Default.DictionaryStringObject));
        // A fixture can be reading the last snapshot when this timer fires.
        // Keep that snapshot and retry on the next tick instead of ending UI.
        try
        {
            File.Move(temporary, path, true);
        }
        catch (Exception ex) when (ex is IOException or UnauthorizedAccessException)
        {
        }
    }

    private (int Spread, string[] Samples) PreviewBackdropProbe() => backdrop?.Probe() ?? (-1, []);

    private void HidePresented()
    {
        if (failureCompletion != null && !stopping) return;
        if (IsVisible) Hide();
        gameCursor.Reset();
        if (raisedForGame && helloPrompts == 0 && WindowNative.IsWindow(handle))
        {
            // Demoting an owned window may also demote its owner. Detach
            // from a topmost game by restoring Avalonia's hidden owner first.
            if (WindowNative.IsTopmost(WindowNative.GetWindow(handle, 4)))
            {
                if (avaloniaOwner == 0) return; // Taskbar preview has no helper owner.
                WindowNative.SetOwner(handle, avaloniaOwner);
            }
            if (!WindowNative.SetWindowPos(handle, -2, 0, 0, 0, 0, 0x213))
                throw new System.ComponentModel.Win32Exception();
            raisedForGame = false;
        }
    }

    private void ResetBackdrop()
    {
        backdropTimer.Stop();
        backdropRefresh.Stop();
        backdropClock.Reset();
        backdropCaptureTimes.Clear();
        backdropCaptureCount = backdropRefreshAttempts = 0;
        backdrop?.Image.Dispose();
        backdrop = null;
        backdropAttempted = false;
        backdropBrush.Source = null;
        cachedBackdrop.Fill = null;
        UpdateMaterialTint();
    }

    private void ScheduleBackdropRefresh()
    {
        backdropTimer.Stop();
        if (NativeUiSettings.HighContrast || !requested || stopping) return;
        if (backdropRefresh.DelayUntilNext(backdropClock.Elapsed) is not TimeSpan delay) return;
        backdropTimer.Interval = delay > TimeSpan.Zero ? delay : TimeSpan.FromSeconds(1);
        backdropTimer.Start();
    }

    private void RefreshBackdrop()
    {
        backdropTimer.Stop();
        if (!requested || stopping || NativeUiSettings.HighContrast || game == null || game.HasExited) return;
        nint foreground = WindowNative.GetForegroundWindow();
        if (!IsVisible || gameWindow == 0 || WindowNative.IsIconic(gameWindow) ||
            (foreground != gameWindow && foreground != handle))
        {
            ScheduleBackdropRefresh();
            return;
        }

        if (backdropRefresh.TryConsumeDue(backdropClock.Elapsed))
        {
            backdropRefreshAttempts++;
            AcrylicBackdrop.Snapshot? refreshed =
                AcrylicBackdrop.Capture(gameWindow, WindowNative.ScreenClient(gameWindow), handle);
            if (refreshed != null)
            {
                AcrylicBackdrop.Snapshot? previousBackdrop = backdrop;
                backdrop = refreshed;
                backdropCaptureCount++;
                backdropCaptureTimes.Add(backdropClock.Elapsed.TotalSeconds);
                backdropBrush.Source = refreshed.Image;
                previousBackdrop?.Image.Dispose();
                cachedBackdrop.Fill = backdropBrush;
                UpdateMaterialTint();
                UpdateBackdropViewbox();
            }
        }

        ScheduleBackdropRefresh();
        if (preview) WritePreviewState();
    }

    private void UpdateMaterialTint()
    {
        bool dark = currentTheme == "night",
            warm = currentTheme == "sunset",
            frosted = backdrop != null && !NativeUiSettings.HighContrast;
        byte opacity = frosted ? (byte)(dark ? 138 : 178) : (byte)248;
        materialTint.Background = new SolidColorBrush(dark ? Color.FromArgb(opacity, 25, 39, 60) :
            warm ? Color.FromArgb(opacity, 252, 237, 218) : Color.FromArgb(opacity, 239, 247, 253));
        card.Background = frosted ? Brushes.Transparent : materialTint.Background;
    }

    private void UpdateBackdropViewbox()
    {
        if (backdrop == null || !IsVisible || Bounds.Width <= 0 || Bounds.Height <= 0) return;
        Point? origin = card.TranslatePoint(default, this);
        Point? corner = card.TranslatePoint(new Point(card.Bounds.Width, card.Bounds.Height), this);
        if (origin == null || corner == null) return;
        double x = Math.Clamp(origin.Value.X / Bounds.Width, 0, .9999),
            y = Math.Clamp(origin.Value.Y / Bounds.Height, 0, .9999);
        backdropBrush.SourceRect = new RelativeRect(x, y,
            Math.Clamp((corner.Value.X - origin.Value.X) / Bounds.Width, .0001, 1 - x),
            Math.Clamp((corner.Value.Y - origin.Value.Y) / Bounds.Height, .0001, 1 - y), RelativeUnit.Relative);
    }

    internal Task ShowFailureAsync(string text)
    {
        requested = false;
        HidePresented();
        ResetBackdrop();
        form.Children.Clear();
        Add(form, new TextBlock { Text = text, TextWrapping = TextWrapping.Wrap, FontSize = 13 });
        modeRail.IsVisible = status.IsVisible = false;
        title.Text = Localization.LoginText.Get("ComponentUnavailable");
        subtitle.Text = Localization.LoginText.Get("CheckStartup");
        subtitle.IsVisible = true;
        failureCompletion = new(TaskCreationOptions.RunContinuationsAsynchronously);
        login.Content = Localization.LoginText.Get("Close");
        otherMainlandAccount.IsVisible = officialLauncher.IsVisible = false;
        login.IsEnabled = true;
        view.Width = SceneWidth;
        view.Height = SceneHeight;
        Width = SceneWidth;
        Height = SceneHeight;
        WindowStartupLocation = WindowStartupLocation.CenterScreen;
        Show();
        Activate();
        return failureCompletion.Task;
    }

    internal void Shutdown()
    {
        stopping = true;
        tracker.Stop();
        smsTimer.Stop();
        CancelFlow();
        welcome?.Stop();
        ResetBackdrop();
        foreach (nint hook in eventHooks) WindowNative.UnhookWinEvent(hook);
        eventHooks.Clear();
        Win32Properties.RemoveWndProcHookCallback(this, NativeMessage);
        Win32Properties.RemoveWindowStylesCallback(this, windowStyles);
        gameCursor.Dispose();
        game?.Dispose();
        Close();
        mascot.Source = null;
        foreach (Bitmap image in mascotImages.Values) image.Dispose();
        mascotImages.Clear();
        GC.KeepAlive(windowEvents);
    }

    private static string Friendly(string phase) => Localization.LoginText.Get(phase switch
    {
        "invalid-mainland-phone" => "InvalidMainlandPhone",
        "invalid-captcha" => "InvalidSmsCode",
        "sms-cooldown" => "SmsCooldown",
        "password-state-unavailable" or "invalid-password-state" => "PasswordStateUnavailable",
        "server-rejected" or "channel-login-rejected" or "game-login-rejected" => "SignInRejected",
        "channel-validation-rejected" => "SavedCredentialsRejected",
        "channel-validation-network-failure" => "SavedValidationNetworkFailure",
        "channel-validation-request-rejected" => "SavedValidationRejected",
        "channel-validation-unknown-response" => "SavedValidationUnknown",
        "channel-validation-official-interaction" => "OfficialInteractionRequired",
        "login-preferences-unavailable" => "ProtectionSettingsUnavailable",
        _ => "SignInServiceUnavailable"
    });
}
