using System.Threading;
using System.Threading.Tasks;

using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.VisualTree;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control;

public partial class MdCadEmbeddedControl : UserControl
{
    public static readonly StyledProperty<string?> JsonlPathProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, string?>(nameof(JsonlPath));

    public static readonly StyledProperty<bool> StartupLiveRefreshEnabledProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(StartupLiveRefreshEnabled));

    public static readonly StyledProperty<bool> ViewportOnlyStartupModeProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(ViewportOnlyStartupMode));

    public static readonly StyledProperty<bool> AutoStartProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(AutoStart), defaultValue: true);

    public static readonly StyledProperty<MdCadPresentationMode> PresentationModeProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, MdCadPresentationMode>(
            nameof(PresentationMode),
            defaultValue: MdCadPresentationMode.Sealed);

    private readonly ContentControl _embedSurfaceContainer;
    private readonly Border _warningSurface;
    private readonly TextBlock _warningTextBlock;
    private readonly Border _diagnosticSurface;
    private readonly Button _diagnosticStartButton;
    private readonly Button _diagnosticStopButton;
    private readonly TextBlock _launchStatusTextBlock;
    private readonly TextBlock _attachStatusTextBlock;
    private readonly TextBlock _jsonlStatusTextBlock;
    private readonly TextBlock _liveRefreshStatusTextBlock;
    private readonly TextBlock _failureTextBlock;
    private readonly MdCadSessionCoordinator _sessionCoordinator;
    private readonly IMdCadEmbedBackend _backend;
    private string? _warningText;
    private bool _isVisualAttached;
    private MdCadLaunchSnapshot _lastLaunchSnapshot;

    static MdCadEmbeddedControl()
    {
        JsonlPathProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.OnLaunchSettingsChanged());
        StartupLiveRefreshEnabledProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.OnLaunchSettingsChanged());
        ViewportOnlyStartupModeProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.OnLaunchSettingsChanged());
        AutoStartProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.OnLaunchSettingsChanged());
        PresentationModeProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.UpdatePresentationMode());
    }

    public MdCadEmbeddedControl()
        : this(null)
    {
    }

    internal MdCadEmbeddedControl(Func<bool>? isWindowsOverride)
    {
        InitializeComponent();

        _embedSurfaceContainer = this.FindControl<ContentControl>("EmbedSurfaceContainer")
            ?? throw new InvalidOperationException("Missing EmbedSurfaceContainer.");
        _warningSurface = this.FindControl<Border>("WarningSurface")
            ?? throw new InvalidOperationException("Missing WarningSurface.");
        _warningTextBlock = this.FindControl<TextBlock>("WarningTextBlock")
            ?? throw new InvalidOperationException("Missing WarningTextBlock.");
        _diagnosticSurface = this.FindControl<Border>("DiagnosticSurface")
            ?? throw new InvalidOperationException("Missing DiagnosticSurface.");
        _diagnosticStartButton = this.FindControl<Button>("DiagnosticStartButton")
            ?? throw new InvalidOperationException("Missing DiagnosticStartButton.");
        _diagnosticStopButton = this.FindControl<Button>("DiagnosticStopButton")
            ?? throw new InvalidOperationException("Missing DiagnosticStopButton.");
        _launchStatusTextBlock = this.FindControl<TextBlock>("LaunchStatusTextBlock")
            ?? throw new InvalidOperationException("Missing LaunchStatusTextBlock.");
        _attachStatusTextBlock = this.FindControl<TextBlock>("AttachStatusTextBlock")
            ?? throw new InvalidOperationException("Missing AttachStatusTextBlock.");
        _jsonlStatusTextBlock = this.FindControl<TextBlock>("JsonlStatusTextBlock")
            ?? throw new InvalidOperationException("Missing JsonlStatusTextBlock.");
        _liveRefreshStatusTextBlock = this.FindControl<TextBlock>("LiveRefreshStatusTextBlock")
            ?? throw new InvalidOperationException("Missing LiveRefreshStatusTextBlock.");
        _failureTextBlock = this.FindControl<TextBlock>("FailureTextBlock")
            ?? throw new InvalidOperationException("Missing FailureTextBlock.");

        _diagnosticStartButton.Click += OnDiagnosticStartClick;
        _diagnosticStopButton.Click += OnDiagnosticStopClick;

        _lastLaunchSnapshot = MdCadLaunchSnapshot.Create(JsonlPath, StartupLiveRefreshEnabled, ViewportOnlyStartupMode);
        AttachedToVisualTree += OnAttachedToVisualTree;
        DetachedFromVisualTree += OnDetachedFromVisualTree;

        _backend = MdCadEmbedBackendFactory.Create(
            initialSnapshot: _lastLaunchSnapshot,
            setLaunchStatus: SetLaunchStatus,
            setAttachStatus: SetAttachStatus,
            updatePreLaunchStatus: UpdatePreLaunchStatus,
            updatePostLaunchStatus: UpdatePostLaunchStatus,
            setFailureStatus: SetFailureStatus,
            setLaunchWarning: SetLaunchWarning,
            updateControlState: UpdateControlState,
            captureLaunchSnapshot: CaptureLaunchSnapshot,
            isWindows: isWindowsOverride);
        _backend.StateChanged += OnBackendStateChanged;
        _backend.UnexpectedSessionLoss += OnBackendUnexpectedSessionLoss;
        _embedSurfaceContainer.Content = _backend.Surface;
        _sessionCoordinator = new MdCadSessionCoordinator(
            captureSnapshot: CaptureLaunchSnapshot,
            canStartSession: CanStartSession,
            getPlaceholderHandle: () => _backend.PlaceholderHandle,
            getAutoStart: () => AutoStart,
            applyWarning: SetLaunchWarning,
            startSessionAsync: (snapshot, cancellationToken) => _backend.StartSessionAsync(snapshot, cancellationToken),
            stopSessionAsync: async cancellationToken =>
            {
                await _backend.StopSessionAsync(cancellationToken);
                if (string.IsNullOrWhiteSpace(_warningText))
                {
                    SetFailureStatus("No host-owned failure.");
                }
            },
            recreateSurfaceAsync: cancellationToken => _backend.RecreateSurfaceAsync(cancellationToken),
            getStartBlockedReason: () => _backend.StartBlockedReason);

        SetLaunchStatus("idle");
        SetAttachStatus("idle");
        RefreshPreSessionPresentation(_lastLaunchSnapshot);
        UpdatePresentationMode();
        UpdateControlState();
    }

    public string? JsonlPath
    {
        get => GetValue(JsonlPathProperty);
        set => SetValue(JsonlPathProperty, value);
    }

    public bool StartupLiveRefreshEnabled
    {
        get => GetValue(StartupLiveRefreshEnabledProperty);
        set => SetValue(StartupLiveRefreshEnabledProperty, value);
    }

    public bool ViewportOnlyStartupMode
    {
        get => GetValue(ViewportOnlyStartupModeProperty);
        set => SetValue(ViewportOnlyStartupModeProperty, value);
    }

    public bool AutoStart
    {
        get => GetValue(AutoStartProperty);
        set => SetValue(AutoStartProperty, value);
    }

    public MdCadPresentationMode PresentationMode
    {
        get => GetValue(PresentationModeProperty);
        set => SetValue(PresentationModeProperty, value);
    }

    public Task StartAsync()
    {
        return RunCoordinatorTaskAsync(_sessionCoordinator.StartAsync(), rethrow: true);
    }

    public Task StopAsync()
    {
        return RunCoordinatorTaskAsync(_sessionCoordinator.StopAsync(), rethrow: true);
    }

    internal IntPtr PlaceholderHandle => _backend.PlaceholderHandle;

    internal EmbedNativeControlHost EmbedSurfaceHost => (EmbedNativeControlHost)_backend.Surface;

    internal void AttachForTesting()
    {
        SetVisualAttachmentState(isAttached: true);
    }

    internal void SetLaunchWarning(string? warningText)
    {
        _warningText = warningText;
        UpdateWarningSurface(warningText);
        if (!string.IsNullOrWhiteSpace(warningText))
        {
            SetFailureStatus(warningText);
        }
        else if (!_backend.HasActiveSession)
        {
            SetFailureStatus("No host-owned failure.");
        }
    }

    private void InitializeComponent()
    {
        AvaloniaXamlLoader.Load(this);
    }

    private MdCadLaunchSnapshot CaptureLaunchSnapshot()
    {
        return MdCadLaunchSnapshot.Create(JsonlPath, StartupLiveRefreshEnabled, ViewportOnlyStartupMode);
    }

    private bool CanStartSession()
    {
        return _isVisualAttached && _backend.CanStartSession;
    }

    private void OnAttachedToVisualTree(object? sender, VisualTreeAttachmentEventArgs e)
    {
        SetVisualAttachmentState(isAttached: true);
    }

    private void OnDetachedFromVisualTree(object? sender, VisualTreeAttachmentEventArgs e)
    {
        SetVisualAttachmentState(isAttached: false);
    }

    private void OnBackendStateChanged()
    {
        if (!ReferenceEquals(_embedSurfaceContainer.Content, _backend.Surface))
        {
            _embedSurfaceContainer.Content = _backend.Surface;
        }

        UpdateControlState();
        QueueReconcile();
    }

    private void OnBackendUnexpectedSessionLoss(string detail)
    {
        SetLaunchWarning(detail);
        UpdateControlState();
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.StopAsync(), rethrow: false);
    }

    private void QueueReconcile()
    {
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.RequestReconcileAsync(), rethrow: false);
    }

    private async Task RunCoordinatorTaskAsync(Task operation, bool rethrow)
    {
        try
        {
            await operation;
        }
        catch (Exception ex)
        {
            SetLaunchStatus("timeout/failure");
            SetAttachStatus("idle");
            UpdatePreLaunchStatus(CaptureLaunchSnapshot());
            SetLaunchWarning(ex.Message);
            UpdateControlState();
            if (rethrow)
            {
                throw;
            }
        }
    }

    private void OnLaunchSettingsChanged()
    {
        _lastLaunchSnapshot = CaptureLaunchSnapshot();
        if (!_backend.HasActiveSession)
        {
            RefreshPreSessionPresentation(_lastLaunchSnapshot);
        }

        UpdateControlState();
        QueueReconcile();
    }

    private void OnDiagnosticStartClick(object? sender, RoutedEventArgs e)
    {
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.StartAsync(), rethrow: false);
    }

    private void OnDiagnosticStopClick(object? sender, RoutedEventArgs e)
    {
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.StopAsync(), rethrow: false);
    }

    private void UpdatePresentationMode()
    {
        _diagnosticSurface.IsVisible = PresentationMode == MdCadPresentationMode.Diagnostic;
    }

    private void UpdateControlState()
    {
        bool canStart = !IsStartBlocked() && CanStartSession() && !_sessionCoordinator.IsSessionRunning;
        bool canStop = _sessionCoordinator.IsSessionRunning || _backend.HasActiveSession;
        _diagnosticStartButton.IsEnabled = canStart;
        _diagnosticStopButton.IsEnabled = canStop;
    }

    private void SetVisualAttachmentState(bool isAttached)
    {
        _isVisualAttached = isAttached;
        RefreshPreSessionPresentation(_lastLaunchSnapshot);
        UpdateControlState();
        QueueReconcile();
    }

    private void RefreshPreSessionPresentation(MdCadLaunchSnapshot snapshot)
    {
        string? primaryWarning = GetPrimaryWarningText(snapshot);
        SetLaunchWarning(primaryWarning);

        if (_backend.HasActiveSession || _sessionCoordinator.IsSessionRunning)
        {
            return;
        }

        if (IsStartBlocked())
        {
            SetLaunchStatus(_isVisualAttached ? "unsupported" : "idle");
            SetAttachStatus(_isVisualAttached ? "unsupported" : "idle");
            UpdateUnsupportedPreLaunchStatus(snapshot);
            return;
        }

        SetLaunchStatus("idle");
        SetAttachStatus("idle");
        UpdatePreLaunchStatus(snapshot);
    }

    private string? GetPrimaryWarningText(MdCadLaunchSnapshot snapshot)
    {
        return _backend.StartBlockedReason ?? snapshot.WarningText;
    }

    private bool IsStartBlocked()
    {
        return !string.IsNullOrWhiteSpace(_backend.StartBlockedReason);
    }

    private void UpdateUnsupportedPreLaunchStatus(MdCadLaunchSnapshot snapshot)
    {
        if (!string.IsNullOrWhiteSpace(snapshot.RequestedJsonlPath))
        {
            SetJsonlStatus($"requested (informational only) -> {snapshot.RequestedJsonlPath}");
        }
        else
        {
            SetJsonlStatus("no startup file requested");
        }

        SetLiveRefreshStatus(snapshot.StartupLiveRefreshEnabled
            ? "requested (informational only)"
            : "off");
    }

    private void UpdatePreLaunchStatus(MdCadLaunchSnapshot snapshot)
    {
        if (snapshot.ShouldPassJsonlArgument)
        {
            SetJsonlStatus($"requested -> {snapshot.LaunchJsonlPath}");
        }
        else if (!string.IsNullOrWhiteSpace(snapshot.RequestedJsonlPath))
        {
            SetJsonlStatus($"warning-only request omitted -> {snapshot.RequestedJsonlPath}");
        }
        else
        {
            SetJsonlStatus("no startup file requested");
        }

        SetLiveRefreshStatus(snapshot.ShouldPassLiveRefreshArgument ? "requested" : "off");
    }

    private void UpdatePostLaunchStatus(MdCadLaunchSnapshot snapshot)
    {
        if (snapshot.ShouldPassJsonlArgument)
        {
            SetJsonlStatus($"viewer-managed after launch (requested: {snapshot.LaunchJsonlPath})");
        }
        else if (!string.IsNullOrWhiteSpace(snapshot.RequestedJsonlPath))
        {
            SetJsonlStatus($"viewer-managed after launch (warning-only request omitted: {snapshot.RequestedJsonlPath})");
        }
        else
        {
            SetJsonlStatus("viewer-managed after launch (no startup file requested)");
        }

        SetLiveRefreshStatus(snapshot.ShouldPassLiveRefreshArgument
            ? "viewer-managed after launch (requested)"
            : "viewer-managed after launch (off)");
    }

    private void SetLaunchStatus(string status)
    {
        _launchStatusTextBlock.Text = $"launch: {status}";
    }

    private void SetAttachStatus(string status)
    {
        _attachStatusTextBlock.Text = $"attach: {status}";
    }

    private void SetJsonlStatus(string status)
    {
        _jsonlStatusTextBlock.Text = $"jsonl: {status}";
    }

    private void SetLiveRefreshStatus(string status)
    {
        _liveRefreshStatusTextBlock.Text = $"live refresh: {status}";
    }

    private void SetFailureStatus(string detail)
    {
        _failureTextBlock.Text = $"detail: {detail}";
    }

    private void UpdateWarningSurface(string? warningText)
    {
        bool hasWarning = !string.IsNullOrWhiteSpace(warningText);
        _warningSurface.IsVisible = hasWarning;
        _warningTextBlock.Text = hasWarning ? warningText! : string.Empty;
    }
}
