using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

using MdCad.Embed.Core;
using MdCad.Wpf.Control.Host;
using MdCad.Wpf.Control.Host.Windows;

namespace MdCad.Wpf.Control;

public partial class MdCadEmbeddedControl : UserControl
{
    public static readonly DependencyProperty JsonlPathProperty =
        DependencyProperty.Register(
            nameof(JsonlPath),
            typeof(string),
            typeof(MdCadEmbeddedControl),
            new PropertyMetadata(null, OnLaunchSettingsChanged));

    public static readonly DependencyProperty StartupLiveRefreshEnabledProperty =
        DependencyProperty.Register(
            nameof(StartupLiveRefreshEnabled),
            typeof(bool),
            typeof(MdCadEmbeddedControl),
            new PropertyMetadata(false, OnLaunchSettingsChanged));

    public static readonly DependencyProperty ViewportOnlyStartupModeProperty =
        DependencyProperty.Register(
            nameof(ViewportOnlyStartupMode),
            typeof(bool),
            typeof(MdCadEmbeddedControl),
            new PropertyMetadata(false, OnLaunchSettingsChanged));

    public static readonly DependencyProperty AutoStartProperty =
        DependencyProperty.Register(
            nameof(AutoStart),
            typeof(bool),
            typeof(MdCadEmbeddedControl),
            new PropertyMetadata(true, OnLaunchSettingsChanged));

    public static readonly DependencyProperty PresentationModeProperty =
        DependencyProperty.Register(
            nameof(PresentationMode),
            typeof(MdCadPresentationMode),
            typeof(MdCadEmbeddedControl),
            new PropertyMetadata(MdCadPresentationMode.Sealed, OnPresentationModeChanged));

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
    private readonly System.Windows.Threading.Dispatcher _dispatcher;
    private string? _jsonlPath;
    private bool _startupLiveRefreshEnabled;
    private bool _viewportOnlyStartupMode;
    private bool _autoStart = true;
    private string? _stickyWarningText;
    private string? _warningText;
    private bool _isVisualAttached;
    private MdCadLaunchSnapshot _lastLaunchSnapshot;

    public MdCadEmbeddedControl()
        : this(null)
    {
    }

    internal MdCadEmbeddedControl(IMdCadEmbedBackend? backend)
    {
        InitializeComponent();
        _dispatcher = Dispatcher;

        _embedSurfaceContainer = (ContentControl)FindName("EmbedSurfaceContainer");
        _warningSurface = (Border)FindName("WarningSurface");
        _warningTextBlock = (TextBlock)FindName("WarningTextBlock");
        _diagnosticSurface = (Border)FindName("DiagnosticSurface");
        _diagnosticStartButton = (Button)FindName("DiagnosticStartButton");
        _diagnosticStopButton = (Button)FindName("DiagnosticStopButton");
        _launchStatusTextBlock = (TextBlock)FindName("LaunchStatusTextBlock");
        _attachStatusTextBlock = (TextBlock)FindName("AttachStatusTextBlock");
        _jsonlStatusTextBlock = (TextBlock)FindName("JsonlStatusTextBlock");
        _liveRefreshStatusTextBlock = (TextBlock)FindName("LiveRefreshStatusTextBlock");
        _failureTextBlock = (TextBlock)FindName("FailureTextBlock");

        _diagnosticStartButton.Click += OnDiagnosticStartClick;
        _diagnosticStopButton.Click += OnDiagnosticStopClick;
        Loaded += OnLoaded;
        Unloaded += OnUnloaded;

        _jsonlPath = (string?)GetValue(JsonlPathProperty);
        _startupLiveRefreshEnabled = (bool)GetValue(StartupLiveRefreshEnabledProperty);
        _viewportOnlyStartupMode = (bool)GetValue(ViewportOnlyStartupModeProperty);
        _autoStart = (bool)GetValue(AutoStartProperty);
        _lastLaunchSnapshot = CaptureLaunchSnapshot();
        _backend = backend ?? MdCadEmbedBackendFactory.Create(
            initialSnapshot: _lastLaunchSnapshot,
            setLaunchStatus: SetLaunchStatus,
            setAttachStatus: SetAttachStatus,
            updatePreLaunchStatus: UpdatePreLaunchStatus,
            updatePostLaunchStatus: UpdatePostLaunchStatus,
            setFailureStatus: SetFailureStatus,
            setLaunchWarning: SetLaunchWarning,
            updateControlState: UpdateControlState,
            captureLaunchSnapshot: CaptureLaunchSnapshot);
        _backend.StateChanged += OnBackendStateChanged;
        _backend.UnexpectedSessionLoss += OnBackendUnexpectedSessionLoss;
        _embedSurfaceContainer.Content = _backend.Surface;
        _sessionCoordinator = new MdCadSessionCoordinator(
            captureSnapshot: CaptureLaunchSnapshot,
            canStartSession: CanStartSession,
            getPlaceholderHandle: () => _backend.PlaceholderHandle,
            getAutoStart: () => _autoStart,
            applyWarning: ApplyCoordinatorWarning,
            startSessionAsync: _backend.StartSessionAsync,
            stopSessionAsync: async cancellationToken =>
            {
                await _backend.StopSessionAsync(cancellationToken);
                if (string.IsNullOrWhiteSpace(_warningText))
                {
                    SetFailureStatus("No host-owned failure.");
                }
            },
            recreateSurfaceAsync: _backend.RecreateSurfaceAsync,
            getStartBlockedReason: () => _backend.StartBlockedReason);

        SetLaunchStatus("idle");
        SetAttachStatus("idle");
        RefreshPreSessionPresentation(_lastLaunchSnapshot);
        UpdatePresentationMode();
        UpdateControlState();
    }

    public string? JsonlPath
    {
        get => (string?)GetValue(JsonlPathProperty);
        set => SetValue(JsonlPathProperty, value);
    }

    public bool StartupLiveRefreshEnabled
    {
        get => (bool)GetValue(StartupLiveRefreshEnabledProperty);
        set => SetValue(StartupLiveRefreshEnabledProperty, value);
    }

    public bool ViewportOnlyStartupMode
    {
        get => (bool)GetValue(ViewportOnlyStartupModeProperty);
        set => SetValue(ViewportOnlyStartupModeProperty, value);
    }

    public bool AutoStart
    {
        get => (bool)GetValue(AutoStartProperty);
        set => SetValue(AutoStartProperty, value);
    }

    public MdCadPresentationMode PresentationMode
    {
        get => (MdCadPresentationMode)GetValue(PresentationModeProperty);
        set => SetValue(PresentationModeProperty, value);
    }

    public event Action<string>? UnexpectedSessionLoss;

    public Task StartAsync()
    {
        _stickyWarningText = null;
        return RunCoordinatorTaskAsync(_sessionCoordinator.StartAsync(), rethrow: true);
    }

    public Task StopAsync()
    {
        _stickyWarningText = null;
        return RunCoordinatorTaskAsync(_sessionCoordinator.StopAsync(), rethrow: true);
    }

    internal void AttachForTesting()
    {
        SetVisualAttachmentState(isAttached: true);
    }

    internal Task RequestReconcileForTestingAsync()
    {
        return RunCoordinatorTaskAsync(_sessionCoordinator.RequestReconcileAsync(), rethrow: true);
    }

    private static void OnLaunchSettingsChanged(DependencyObject dependencyObject, DependencyPropertyChangedEventArgs _)
    {
        if (dependencyObject is MdCadEmbeddedControl control)
        {
            control.SyncLaunchSettingCache();
            control.OnLaunchSettingsChanged();
        }
    }

    private static void OnPresentationModeChanged(DependencyObject dependencyObject, DependencyPropertyChangedEventArgs _)
    {
        if (dependencyObject is MdCadEmbeddedControl control)
        {
            control.UpdatePresentationMode();
        }
    }

    private MdCadLaunchSnapshot CaptureLaunchSnapshot()
    {
        return MdCadLaunchSnapshot.Create(_jsonlPath, _startupLiveRefreshEnabled, _viewportOnlyStartupMode);
    }

    private bool CanStartSession()
    {
        return _isVisualAttached && _backend.CanStartSession;
    }

    private async Task RunCoordinatorTaskAsync(Task operation, bool rethrow)
    {
        try
        {
            await operation;
        }
        catch (Exception ex)
        {
            _stickyWarningText = ex.Message;
            SetLaunchStatus("timeout/failure");
            SetAttachStatus("idle");
            UpdatePreLaunchStatus(_lastLaunchSnapshot);
            SetLaunchWarning(ex.Message);
            UpdateControlState();
            if (rethrow)
            {
                throw;
            }
        }
    }

    private void OnLoaded(object sender, RoutedEventArgs e)
    {
        SetVisualAttachmentState(isAttached: true);
    }

    private void OnUnloaded(object sender, RoutedEventArgs e)
    {
        SetVisualAttachmentState(isAttached: false);
    }

    private void OnBackendStateChanged()
    {
        UpdateUi(() =>
        {
            if (!ReferenceEquals(_embedSurfaceContainer.Content, _backend.Surface))
            {
                _embedSurfaceContainer.Content = _backend.Surface;
            }
        });

        UpdateControlState();
        QueueReconcile();
    }

    private void OnBackendUnexpectedSessionLoss(string detail)
    {
        _stickyWarningText = detail;
        SetLaunchWarning(detail);
        UpdateControlState();
        UnexpectedSessionLoss?.Invoke(detail);
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.StopAsync(), rethrow: false);
    }

    private void QueueReconcile()
    {
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.RequestReconcileAsync(), rethrow: false);
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

    private void ApplyCoordinatorWarning(string? warningText)
    {
        SetLaunchWarning(warningText ?? _stickyWarningText);
    }

    private void OnDiagnosticStartClick(object sender, RoutedEventArgs e)
    {
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.StartAsync(), rethrow: false);
    }

    private void OnDiagnosticStopClick(object sender, RoutedEventArgs e)
    {
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.StopAsync(), rethrow: false);
    }

    private void UpdatePresentationMode()
    {
        UpdateUi(() =>
        {
            _diagnosticSurface.Visibility = PresentationMode == MdCadPresentationMode.Diagnostic
                ? Visibility.Visible
                : Visibility.Collapsed;
        });
    }

    private void UpdateControlState()
    {
        if (_diagnosticStartButton is null || _diagnosticStopButton is null)
        {
            return;
        }

        UpdateUi(() =>
        {
            bool canStart = !IsStartBlocked() && CanStartSession() && !_sessionCoordinator.IsSessionRunning;
            bool canStop = _sessionCoordinator.IsSessionRunning || _backend.HasActiveSession;
            _diagnosticStartButton.IsEnabled = canStart;
            _diagnosticStopButton.IsEnabled = canStop;
        });
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
        return _backend.StartBlockedReason ?? _stickyWarningText ?? snapshot.WarningText;
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

        SetLiveRefreshStatus(snapshot.StartupLiveRefreshEnabled ? "requested (informational only)" : "off");
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
        UpdateUi(() => _launchStatusTextBlock.Text = $"launch: {status}");
    }

    private void SetAttachStatus(string status)
    {
        UpdateUi(() => _attachStatusTextBlock.Text = $"attach: {status}");
    }

    private void SetJsonlStatus(string status)
    {
        UpdateUi(() => _jsonlStatusTextBlock.Text = $"jsonl: {status}");
    }

    private void SetLiveRefreshStatus(string status)
    {
        UpdateUi(() => _liveRefreshStatusTextBlock.Text = $"live refresh: {status}");
    }

    private void SetFailureStatus(string detail)
    {
        UpdateUi(() => _failureTextBlock.Text = $"detail: {detail}");
    }

    private void SetLaunchWarning(string? warningText)
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

    private void UpdateWarningSurface(string? warningText)
    {
        UpdateUi(() =>
        {
            bool hasWarning = !string.IsNullOrWhiteSpace(warningText);
            _warningSurface.Visibility = hasWarning ? Visibility.Visible : Visibility.Collapsed;
            _warningTextBlock.Text = hasWarning ? warningText! : string.Empty;
        });
    }

    private void UpdateUi(Action update)
    {
        if (_dispatcher.CheckAccess())
        {
            update();
            return;
        }

        try
        {
            _dispatcher.BeginInvoke(update);
        }
        catch (TaskCanceledException)
        {
        }
        catch (InvalidOperationException)
        {
        }
    }

    private void SyncLaunchSettingCache()
    {
        _jsonlPath = (string?)GetValue(JsonlPathProperty);
        _startupLiveRefreshEnabled = (bool)GetValue(StartupLiveRefreshEnabledProperty);
        _viewportOnlyStartupMode = (bool)GetValue(ViewportOnlyStartupModeProperty);
        _autoStart = (bool)GetValue(AutoStartProperty);
    }
}
