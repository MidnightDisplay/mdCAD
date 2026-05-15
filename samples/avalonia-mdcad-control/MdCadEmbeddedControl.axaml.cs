using System.ComponentModel;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;

using Avalonia;
using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Avalonia.VisualTree;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control;

public partial class MdCadEmbeddedControl : UserControl
{
    private static readonly TimeSpan GracefulEmbeddedExitWait = TimeSpan.FromSeconds(3);
    private static readonly TimeSpan ChildAttachTimeout = TimeSpan.FromSeconds(10);

    public static readonly StyledProperty<string?> JsonlPathProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, string?>(nameof(JsonlPath));

    public static readonly StyledProperty<bool> StartupLiveRefreshEnabledProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(StartupLiveRefreshEnabled));

    public static readonly StyledProperty<bool> AutoStartProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(AutoStart), defaultValue: true);

    public static readonly StyledProperty<MdCadPresentationMode> PresentationModeProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, MdCadPresentationMode>(
            nameof(PresentationMode),
            defaultValue: MdCadPresentationMode.Sealed);

    private readonly ContentControl _embedSurfaceContainer;
    private EmbedNativeControlHost _embedSurfaceHost;
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
    private readonly DispatcherTimer _attachTimer;
    private readonly DispatcherTimer _resizeSyncTimer;
    private readonly MdCadSessionCoordinator _sessionCoordinator;
    private Process? _mdcadProcess;
    private IntPtr _placeholderHandle;
    private IntPtr _launchParentHwnd;
    private IntPtr _attachedChildHwnd;
    private DateTimeOffset _launchStartedAt;
    private string? _capturedFailureLine;
    private string? _warningText;
    private bool _isVisualAttached;
    private MdCadLaunchSnapshot _lastLaunchSnapshot;

    static MdCadEmbeddedControl()
    {
        JsonlPathProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.OnLaunchSettingsChanged());
        StartupLiveRefreshEnabledProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.OnLaunchSettingsChanged());
        AutoStartProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.OnLaunchSettingsChanged());
        PresentationModeProperty.Changed.AddClassHandler<MdCadEmbeddedControl>((control, _) => control.UpdatePresentationMode());
    }

    public MdCadEmbeddedControl()
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

        _lastLaunchSnapshot = MdCadLaunchSnapshot.Create(JsonlPath, StartupLiveRefreshEnabled);
        _attachTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };
        _resizeSyncTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(33) };
        _attachTimer.Tick += OnAttachTimerTick;
        _resizeSyncTimer.Tick += OnResizeSyncTick;
        AttachedToVisualTree += OnAttachedToVisualTree;
        DetachedFromVisualTree += OnDetachedFromVisualTree;

        _embedSurfaceHost = CreateEmbedSurfaceHost();
        _sessionCoordinator = new MdCadSessionCoordinator(
            captureSnapshot: CaptureLaunchSnapshot,
            canStartSession: CanStartSession,
            getPlaceholderHandle: () => _placeholderHandle,
            getAutoStart: () => AutoStart,
            applyWarning: SetLaunchWarning,
            startSessionAsync: StartEmbeddedSessionAsync,
            stopSessionAsync: StopEmbeddedSessionAsync,
            recreateSurfaceAsync: RecreateEmbedSurfaceAsync);

        SetLaunchStatus("idle");
        SetAttachStatus("idle");
        UpdatePreLaunchStatus(_lastLaunchSnapshot);
        SetFailureStatus("No host-owned failure.");
        UpdateWarningSurface(null);
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

    internal IntPtr PlaceholderHandle => _placeholderHandle;

    internal EmbedNativeControlHost EmbedSurfaceHost => _embedSurfaceHost;

    internal void SetLaunchWarning(string? warningText)
    {
        _warningText = warningText;
        UpdateWarningSurface(warningText);
        if (!string.IsNullOrWhiteSpace(warningText))
        {
            SetFailureStatus(warningText);
        }
        else if (_mdcadProcess == null && _attachedChildHwnd == IntPtr.Zero)
        {
            SetFailureStatus("No host-owned failure.");
        }
    }

    private void InitializeComponent()
    {
        AvaloniaXamlLoader.Load(this);
    }

    private EmbedNativeControlHost CreateEmbedSurfaceHost()
    {
        EmbedNativeControlHost host = new();
        host.PlaceholderHandleReady += OnPlaceholderHandleReady;
        host.SizeChanged += OnEmbedSurfaceSizeChanged;
        _embedSurfaceContainer.Content = host;
        return host;
    }

    private MdCadLaunchSnapshot CaptureLaunchSnapshot()
    {
        return MdCadLaunchSnapshot.Create(JsonlPath, StartupLiveRefreshEnabled);
    }

    private bool CanStartSession()
    {
        return _isVisualAttached &&
               _placeholderHandle != IntPtr.Zero &&
               Win32NativeMethods.TryGetClientSize(_placeholderHandle, out int width, out int height) &&
               width > 4 &&
               height > 4;
    }

    private async Task StartEmbeddedSessionAsync(MdCadLaunchSnapshot snapshot, IntPtr placeholderHandle, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_mdcadProcess != null)
        {
            throw new InvalidOperationException("An mdCAD session is already running.");
        }

        MdCadRuntimePaths runtime = MdCadRuntimeResolver.Resolve();
        _lastLaunchSnapshot = snapshot;
        SetLaunchStatus("launching");
        SetAttachStatus("waiting for child attach");
        UpdatePreLaunchStatus(snapshot);
        SetFailureStatus($"runtime: {runtime.ExecutablePath}; parent hwnd: 0x{placeholderHandle.ToInt64():X}");
        UpdateControlState();

        ProcessStartInfo startInfo = new()
        {
            FileName = runtime.ExecutablePath,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = false,
            WorkingDirectory = runtime.RuntimeRoot,
        };
        startInfo.ArgumentList.Add("--embedded");
        startInfo.ArgumentList.Add("--parent-hwnd");
        startInfo.ArgumentList.Add($"0x{placeholderHandle.ToInt64():X}");
        if (snapshot.ShouldPassJsonlArgument)
        {
            startInfo.ArgumentList.Add("--jsonl");
            startInfo.ArgumentList.Add(snapshot.LaunchJsonlPath!);
        }
        if (snapshot.ShouldPassLiveRefreshArgument)
        {
            startInfo.ArgumentList.Add("--jsonl-live-refresh");
        }

        Process process = new()
        {
            StartInfo = startInfo,
            EnableRaisingEvents = false,
        };
        process.OutputDataReceived += OnChildOutputDataReceived;
        process.ErrorDataReceived += OnChildOutputDataReceived;

        try
        {
            if (!process.Start())
            {
                throw new InvalidOperationException("Failed to start mdCAD.");
            }
        }
        catch (Win32Exception ex)
        {
            process.OutputDataReceived -= OnChildOutputDataReceived;
            process.ErrorDataReceived -= OnChildOutputDataReceived;
            process.Dispose();
            throw new InvalidOperationException($"Failed to start mdCAD: {ex.Message}", ex);
        }
        catch (InvalidOperationException)
        {
            process.OutputDataReceived -= OnChildOutputDataReceived;
            process.ErrorDataReceived -= OnChildOutputDataReceived;
            process.Dispose();
            throw;
        }

        _mdcadProcess = process;
        _capturedFailureLine = null;
        _attachedChildHwnd = IntPtr.Zero;
        _launchParentHwnd = placeholderHandle;
        _launchStartedAt = DateTimeOffset.UtcNow;
        _mdcadProcess.BeginOutputReadLine();
        _mdcadProcess.BeginErrorReadLine();
        _attachTimer.Start();
        UpdateControlState();
    }

    private async Task StopEmbeddedSessionAsync(CancellationToken cancellationToken)
    {
        _attachTimer.Stop();
        _resizeSyncTimer.Stop();

        bool hadSession = _mdcadProcess != null || _attachedChildHwnd != IntPtr.Zero;
        if (hadSession)
        {
            SetLaunchStatus("teardown/cleanup");
            SetFailureStatus("Host requested stop/close.");
            UpdateControlState();
        }

        IntPtr placeholderToDestroy = _placeholderHandle;
        _placeholderHandle = IntPtr.Zero;
        _embedSurfaceHost.ClearPlaceholderHandle();
        if (placeholderToDestroy != IntPtr.Zero && Win32NativeMethods.IsWindow(placeholderToDestroy))
        {
            Win32NativeMethods.DestroyWindow(placeholderToDestroy);
        }

        _attachedChildHwnd = IntPtr.Zero;
        _launchParentHwnd = IntPtr.Zero;
        _capturedFailureLine = null;

        if (_mdcadProcess != null)
        {
            Process process = _mdcadProcess;
            if (!process.HasExited)
            {
                bool exitedGracefully = await WaitForExitAsync(process, GracefulEmbeddedExitWait, cancellationToken);
                if (!exitedGracefully && !process.HasExited)
                {
                    try
                    {
                        process.Kill(true);
                        await WaitForExitAsync(process, GracefulEmbeddedExitWait, cancellationToken);
                    }
                    catch (InvalidOperationException)
                    {
                        // Process already exited while fallback cleanup was starting.
                    }
                }
            }
        }

        DisposeCurrentProcess();
        SetLaunchStatus("idle");
        SetAttachStatus("idle");
        UpdatePreLaunchStatus(CaptureLaunchSnapshot());
        if (string.IsNullOrWhiteSpace(_warningText))
        {
            SetFailureStatus("No host-owned failure.");
        }
        UpdateControlState();
    }

    private Task RecreateEmbedSurfaceAsync(CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();

        _embedSurfaceHost.PlaceholderHandleReady -= OnPlaceholderHandleReady;
        _embedSurfaceHost.SizeChanged -= OnEmbedSurfaceSizeChanged;
        _placeholderHandle = IntPtr.Zero;
        _embedSurfaceHost = CreateEmbedSurfaceHost();
        SetAttachStatus("idle");
        UpdateControlState();
        return Task.CompletedTask;
    }

    private void OnAttachedToVisualTree(object? sender, VisualTreeAttachmentEventArgs e)
    {
        _isVisualAttached = true;
        UpdateControlState();
        QueueReconcile();
    }

    private void OnDetachedFromVisualTree(object? sender, VisualTreeAttachmentEventArgs e)
    {
        _isVisualAttached = false;
        UpdateControlState();
        QueueReconcile();
    }

    private void OnPlaceholderHandleReady(IntPtr hwnd)
    {
        _placeholderHandle = hwnd;
        UpdateControlState();
        QueueReconcile();
    }

    private void OnEmbedSurfaceSizeChanged(object? sender, SizeChangedEventArgs e)
    {
        if (_attachedChildHwnd != IntPtr.Zero)
        {
            SyncAttachedChildBounds();
        }

        UpdateControlState();
        QueueReconcile();
    }

    private void OnAttachTimerTick(object? sender, EventArgs e)
    {
        if (_attachedChildHwnd != IntPtr.Zero)
        {
            _attachTimer.Stop();
            return;
        }

        if (_mdcadProcess == null)
        {
            _attachTimer.Stop();
            return;
        }

        if (_mdcadProcess.HasExited)
        {
            _attachTimer.Stop();
            CleanupAfterUnexpectedSessionLoss(
                _capturedFailureLine ?? $"mdCAD exited before child attach (exit code {_mdcadProcess.ExitCode}).");
            return;
        }

        IntPtr childHwnd = FindChildWindowForProcess(_launchParentHwnd, _mdcadProcess.Id);
        if (childHwnd != IntPtr.Zero)
        {
            _attachedChildHwnd = childHwnd;
            _attachTimer.Stop();
            SyncAttachedChildBounds();
            _resizeSyncTimer.Start();
            SetLaunchStatus("active");
            SetAttachStatus("attached");
            UpdatePostLaunchStatus(_lastLaunchSnapshot);
            SetFailureStatus($"child hwnd attached: 0x{childHwnd.ToInt64():X}");
            UpdateControlState();
            return;
        }

        if ((DateTimeOffset.UtcNow - _launchStartedAt) >= ChildAttachTimeout)
        {
            _attachTimer.Stop();
            CleanupAfterUnexpectedSessionLoss(_capturedFailureLine ?? "Timed out waiting for child attach.");
        }
    }

    private void OnResizeSyncTick(object? sender, EventArgs e)
    {
        if (_attachedChildHwnd == IntPtr.Zero)
        {
            _resizeSyncTimer.Stop();
            return;
        }

        if (_mdcadProcess?.HasExited == true)
        {
            CleanupAfterUnexpectedSessionLoss($"mdCAD exited after attach (exit code {_mdcadProcess.ExitCode}).");
            return;
        }

        if (!EnsureAttachedLifecycleIsHealthy())
        {
            return;
        }

        SyncAttachedChildBounds();
    }

    private bool EnsureAttachedLifecycleIsHealthy()
    {
        if (_attachedChildHwnd == IntPtr.Zero)
        {
            return false;
        }

        if (_placeholderHandle == IntPtr.Zero ||
            !Win32NativeMethods.IsWindow(_placeholderHandle) ||
            !Win32NativeMethods.IsWindow(_attachedChildHwnd) ||
            Win32NativeMethods.GetParent(_attachedChildHwnd) != _placeholderHandle)
        {
            CleanupAfterUnexpectedSessionLoss("Attached child HWND is no longer parented to the placeholder.");
            return false;
        }

        return true;
    }

    private void SyncAttachedChildBounds()
    {
        if (_placeholderHandle == IntPtr.Zero ||
            _attachedChildHwnd == IntPtr.Zero ||
            !Win32NativeMethods.TryGetClientSize(_placeholderHandle, out int width, out int height))
        {
            return;
        }

        Win32NativeMethods.MoveWindow(_attachedChildHwnd, 0, 0, width, height, true);
    }

    private void OnChildOutputDataReceived(object sender, DataReceivedEventArgs e)
    {
        if (string.IsNullOrWhiteSpace(e.Data))
        {
            return;
        }

        if (_capturedFailureLine == null && e.Data.Contains("Embedded startup failed:", StringComparison.Ordinal))
        {
            _capturedFailureLine = e.Data.Trim();
            Dispatcher.UIThread.Post(() =>
            {
                if (_attachedChildHwnd == IntPtr.Zero)
                {
                    SetLaunchWarning(_capturedFailureLine);
                }
            });
        }
    }

    private void CleanupAfterUnexpectedSessionLoss(string detail)
    {
        _attachTimer.Stop();
        _resizeSyncTimer.Stop();
        DisposeCurrentProcess();
        _attachedChildHwnd = IntPtr.Zero;
        _launchParentHwnd = IntPtr.Zero;
        _capturedFailureLine = null;
        SetLaunchStatus("timeout/failure");
        SetAttachStatus("idle");
        UpdatePreLaunchStatus(CaptureLaunchSnapshot());
        SetLaunchWarning(detail);
        UpdateControlState();
        _ = RunCoordinatorTaskAsync(_sessionCoordinator.StopAsync(), rethrow: false);
    }

    private void DisposeCurrentProcess()
    {
        if (_mdcadProcess == null)
        {
            return;
        }

        _mdcadProcess.OutputDataReceived -= OnChildOutputDataReceived;
        _mdcadProcess.ErrorDataReceived -= OnChildOutputDataReceived;
        _mdcadProcess.Dispose();
        _mdcadProcess = null;
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
        if (_mdcadProcess == null && _attachedChildHwnd == IntPtr.Zero)
        {
            UpdatePreLaunchStatus(_lastLaunchSnapshot);
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
        bool canStart = CanStartSession() && !_sessionCoordinator.IsSessionRunning;
        bool canStop = _sessionCoordinator.IsSessionRunning || _mdcadProcess != null;
        _diagnosticStartButton.IsEnabled = canStart;
        _diagnosticStopButton.IsEnabled = canStop;
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

    private static async Task<bool> WaitForExitAsync(Process process, TimeSpan timeout, CancellationToken cancellationToken)
    {
        try
        {
            using CancellationTokenSource timeoutSource = new(timeout);
            using CancellationTokenSource linkedSource = CancellationTokenSource.CreateLinkedTokenSource(
                cancellationToken,
                timeoutSource.Token);
            await process.WaitForExitAsync(linkedSource.Token);
            return true;
        }
        catch (OperationCanceledException)
        {
            return process.HasExited;
        }
        catch (InvalidOperationException)
        {
            return process.HasExited;
        }
    }

    private static IntPtr FindChildWindowForProcess(IntPtr parentHwnd, int processId)
    {
        if (!OperatingSystem.IsWindows() || parentHwnd == IntPtr.Zero || !Win32NativeMethods.IsWindow(parentHwnd))
        {
            return IntPtr.Zero;
        }

        IntPtr matchedChild = IntPtr.Zero;
        Win32NativeMethods.EnumChildWindows(parentHwnd, (hwnd, _) =>
        {
            Win32NativeMethods.GetWindowThreadProcessId(hwnd, out uint windowProcessId);
            if (windowProcessId == (uint)processId)
            {
                matchedChild = hwnd;
                return false;
            }

            return true;
        }, IntPtr.Zero);

        return matchedChild;
    }

    private void UpdateWarningSurface(string? warningText)
    {
        bool hasWarning = !string.IsNullOrWhiteSpace(warningText);
        _warningSurface.IsVisible = hasWarning;
        _warningTextBlock.Text = hasWarning ? warningText! : string.Empty;
    }
}
