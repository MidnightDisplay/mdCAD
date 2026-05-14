using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

using Avalonia.Controls;
using Avalonia.Input;
using Avalonia.Markup.Xaml;
using Avalonia.Platform;
using Avalonia.Threading;

namespace AvaloniaHost;

public enum EmbedTestMode
{
    Valid,
    InvalidParent,
    DestroyedParent,
    DestroyAfterAttach,
}

public sealed class EmbedNativeControlHost : NativeControlHost
{
    public event Action<IntPtr>? PlaceholderHandleReady;

    public IntPtr PlaceholderHandle { get; private set; }

    public void ClearPlaceholderHandle()
    {
        PlaceholderHandle = IntPtr.Zero;
    }

    protected override IPlatformHandle CreateNativeControlCore(IPlatformHandle parent)
    {
        if (!OperatingSystem.IsWindows())
        {
            return base.CreateNativeControlCore(parent);
        }

        IntPtr parentHandle = parent.Handle;
        if (parentHandle == IntPtr.Zero)
        {
            throw new InvalidOperationException("Expected a valid Win32 parent handle for NativeControlHost.");
        }

        PlaceholderHandle = Win32NativeMethods.CreatePlaceholderWindow(parentHandle);
        PlaceholderHandleReady?.Invoke(PlaceholderHandle);
        return new PlatformHandle(PlaceholderHandle, "HWND");
    }

    protected override void DestroyNativeControlCore(IPlatformHandle control)
    {
        if (OperatingSystem.IsWindows() && control.Handle != IntPtr.Zero)
        {
            Win32NativeMethods.DestroyWindow(control.Handle);
            if (PlaceholderHandle == control.Handle)
            {
                PlaceholderHandle = IntPtr.Zero;
            }
            return;
        }

        base.DestroyNativeControlCore(control);
    }
}

public sealed class HostLaunchOptions
{
    public string? MdcadExePath { get; }
    public EmbedTestMode TestMode { get; }
    public string? ParseError { get; }

    public string TestModeToken => TestMode switch
    {
        EmbedTestMode.Valid => "valid",
        EmbedTestMode.InvalidParent => "invalid-parent",
        EmbedTestMode.DestroyedParent => "destroyed-parent",
        EmbedTestMode.DestroyAfterAttach => "destroy-after-attach",
        _ => "valid",
    };

    private HostLaunchOptions(string? mdcadExePath, EmbedTestMode testMode, string? parseError)
    {
        MdcadExePath = mdcadExePath;
        TestMode = testMode;
        ParseError = parseError;
    }

    public static HostLaunchOptions Parse(string[] args)
    {
        string? mdcadExePath = null;
        EmbedTestMode testMode = EmbedTestMode.Valid;
        string? parseError = null;

        for (int i = 0; i < args.Length; ++i)
        {
            string arg = args[i];
            if (string.Equals(arg, "--mdcad-exe", StringComparison.Ordinal))
            {
                if ((i + 1) >= args.Length || string.IsNullOrWhiteSpace(args[i + 1]))
                {
                    parseError = "Missing value for --mdcad-exe";
                    break;
                }
                mdcadExePath = args[++i];
                continue;
            }

            if (string.Equals(arg, "--embed-test-mode", StringComparison.Ordinal))
            {
                if ((i + 1) >= args.Length || string.IsNullOrWhiteSpace(args[i + 1]))
                {
                    parseError = "Missing value for --embed-test-mode";
                    break;
                }

                string modeToken = args[++i];
                if (string.Equals(modeToken, "valid", StringComparison.Ordinal))
                {
                    testMode = EmbedTestMode.Valid;
                }
                else if (string.Equals(modeToken, "invalid-parent", StringComparison.Ordinal))
                {
                    testMode = EmbedTestMode.InvalidParent;
                }
                else if (string.Equals(modeToken, "destroyed-parent", StringComparison.Ordinal))
                {
                    testMode = EmbedTestMode.DestroyedParent;
                }
                else if (string.Equals(modeToken, "destroy-after-attach", StringComparison.Ordinal))
                {
                    testMode = EmbedTestMode.DestroyAfterAttach;
                }
                else
                {
                    parseError = $"Unsupported --embed-test-mode value: {modeToken}";
                    break;
                }
            }
        }

        return new HostLaunchOptions(mdcadExePath, testMode, parseError);
    }
}

public partial class MainWindow : Window
{
    private const string StatusLaunching = "launching";
    private const string StatusWaitingForChildAttach = "waiting for child attach";
    private const string StatusAttached = "attached";
    private const string StatusTeardownCleanup = "teardown/cleanup";
    private const string StatusTimeoutFailure = "timeout/failure";
    private static readonly TimeSpan GracefulEmbeddedExitWait = TimeSpan.FromSeconds(3);

    private readonly HostLaunchOptions _options;
    private readonly EmbedNativeControlHost _embedSurface;
    private readonly TextBlock _statusTextBlock;
    private readonly TextBlock _modeTextBlock;
    private readonly TextBlock _failureTextBlock;
    private readonly DispatcherTimer _attachTimer;
    private readonly DispatcherTimer _launchRetryTimer;
    private readonly DispatcherTimer _resizeSyncTimer;
    private Process? _mdcadProcess;
    private IntPtr _placeholderHwnd;
    private IntPtr _launchParentHwnd;
    private IntPtr _attachedChildHwnd;
    private DateTimeOffset _launchStartedAt;
    private bool _launchStarted;
    private string? _capturedFailureLine;
    private string? _teardownReason;
    private bool _destroyAfterAttachTriggered;
    private bool _fallbackKillIssued;
    private bool _teardownAwaitingProcessExit;

    public MainWindow()
        : this(HostLaunchOptions.Parse(Array.Empty<string>()))
    {
    }

    public MainWindow(HostLaunchOptions options)
    {
        _options = options;
        InitializeComponent();

        _embedSurface = this.FindControl<EmbedNativeControlHost>("EmbedSurface")
            ?? throw new InvalidOperationException("Missing EmbedSurface control.");
        _statusTextBlock = this.FindControl<TextBlock>("StatusTextBlock")
            ?? throw new InvalidOperationException("Missing StatusTextBlock.");
        _modeTextBlock = this.FindControl<TextBlock>("ModeTextBlock")
            ?? throw new InvalidOperationException("Missing ModeTextBlock.");
        _failureTextBlock = this.FindControl<TextBlock>("FailureTextBlock")
            ?? throw new InvalidOperationException("Missing FailureTextBlock.");
        _attachTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };
        _launchRetryTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };
        _resizeSyncTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(33) };

        _embedSurface.PlaceholderHandleReady += OnPlaceholderHandleReady;
        _embedSurface.SizeChanged += OnEmbedSurfaceSizeChanged;
        _attachTimer.Tick += OnAttachTimerTick;
        _launchRetryTimer.Tick += OnLaunchRetryTick;
        _resizeSyncTimer.Tick += OnResizeSyncTick;
        Closed += OnWindowClosed;

        _statusTextBlock.Text = StatusLaunching;
        _modeTextBlock.Text = $"Harness mode: {_options.TestModeToken}";
        _failureTextBlock.Text = BuildScaffoldMessage(IntPtr.Zero);

        if (!string.IsNullOrWhiteSpace(_options.ParseError))
        {
            _statusTextBlock.Text = StatusTimeoutFailure;
            _failureTextBlock.Text = _options.ParseError;
        }
    }

    private void InitializeComponent()
    {
        AvaloniaXamlLoader.Load(this);
    }

    private void OnPlaceholderHandleReady(IntPtr hwnd)
    {
        if (!string.IsNullOrWhiteSpace(_options.ParseError))
        {
            _statusTextBlock.Text = StatusTimeoutFailure;
            return;
        }

        if (_launchStarted)
        {
            return;
        }

        _placeholderHwnd = hwnd;
        _failureTextBlock.Text = BuildScaffoldMessage(hwnd);
        TryLaunchWhenSurfaceReady();
    }

    private string BuildScaffoldMessage(IntPtr hwnd)
    {
        string pathText = string.IsNullOrWhiteSpace(_options.MdcadExePath)
            ? "mdCAD path: deferred relative resolution"
            : $"mdCAD path override: {_options.MdcadExePath}";
        string hwndText = hwnd == IntPtr.Zero
            ? "Host HWND: pending NativeControlHost creation"
            : $"Host HWND: 0x{hwnd.ToInt64():X}";

        return $"{pathText}{Environment.NewLine}{hwndText}{Environment.NewLine}"
             + "Wave 0 scaffold reserves --mdcad-exe, --embed-test-mode valid|invalid-parent|destroyed-parent|destroy-after-attach, "
             + "and the staged statuses launching, waiting for child attach, attached, teardown/cleanup, timeout/failure.";
    }

    private IntPtr PrepareLaunchParentHwnd(IntPtr placeholderHwnd)
    {
        if (_options.TestMode == EmbedTestMode.InvalidParent)
        {
            return new IntPtr(0x1);
        }

        return placeholderHwnd;
    }

    private void OnLaunchRetryTick(object? sender, EventArgs e)
    {
        TryLaunchWhenSurfaceReady();
    }

    private void TryLaunchWhenSurfaceReady()
    {
        if (_launchStarted || _placeholderHwnd == IntPtr.Zero)
        {
            return;
        }

        if (!Win32NativeMethods.TryGetClientSize(_placeholderHwnd, out int width, out int height) || width <= 4 || height <= 4)
        {
            _statusTextBlock.Text = StatusLaunching;
            _failureTextBlock.Text =
                $"Waiting for host surface size...{Environment.NewLine}" +
                $"Current placeholder HWND: 0x{_placeholderHwnd.ToInt64():X}";
            _launchRetryTimer.Start();
            return;
        }

        _launchRetryTimer.Stop();
        _launchStarted = true;
        _launchParentHwnd = PrepareLaunchParentHwnd(_placeholderHwnd);
        LaunchEmbeddedViewer();
    }

    private void LaunchEmbeddedViewer()
    {
        if (!TryResolveMdcadExecutable(out string? mdCadExePath, out string failureMessage))
        {
            SetFailureStatus(failureMessage);
            return;
        }

        _statusTextBlock.Text = StatusLaunching;
        _failureTextBlock.Text =
            $"mdCAD path: {mdCadExePath}{Environment.NewLine}" +
            $"Parent HWND: 0x{_launchParentHwnd.ToInt64():X}{Environment.NewLine}" +
            $"Mode: {_options.TestModeToken}";

        ProcessStartInfo startInfo = new()
        {
            FileName = mdCadExePath,
            Arguments = $"--embedded --parent-hwnd 0x{_launchParentHwnd.ToInt64():X}",
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = false,
            WorkingDirectory = Path.GetDirectoryName(mdCadExePath) ?? Directory.GetCurrentDirectory(),
        };

        try
        {
            _mdcadProcess = new Process
            {
                StartInfo = startInfo,
                EnableRaisingEvents = true,
            };
            _mdcadProcess.OutputDataReceived += OnChildOutputDataReceived;
            _mdcadProcess.ErrorDataReceived += OnChildOutputDataReceived;
            _mdcadProcess.Exited += OnChildProcessExited;

            if (!_mdcadProcess.Start())
            {
                SetFailureStatus("Failed to start mdCAD.");
                return;
            }
        }
        catch (Win32Exception ex)
        {
            SetFailureStatus($"Failed to start mdCAD: {ex.Message}");
            return;
        }
        catch (InvalidOperationException ex)
        {
            SetFailureStatus($"Failed to start mdCAD: {ex.Message}");
            return;
        }

        _mdcadProcess.BeginOutputReadLine();
        _mdcadProcess.BeginErrorReadLine();

        _launchStartedAt = DateTimeOffset.UtcNow;
        _statusTextBlock.Text = StatusWaitingForChildAttach;
        _modeTextBlock.Text = $"Harness mode: {_options.TestModeToken}";
        _attachTimer.Start();
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
                    _failureTextBlock.Text = _capturedFailureLine;
                }
            });
        }
    }

    private void OnChildProcessExited(object? sender, EventArgs e)
    {
        Dispatcher.UIThread.Post(() =>
        {
            int exitCode = _mdcadProcess?.HasExited == true ? _mdcadProcess.ExitCode : -1;
            if (!string.IsNullOrWhiteSpace(_teardownReason) || _fallbackKillIssued)
            {
                FinalizeTeardownStatus(exitCode);
                return;
            }

            if (_attachedChildHwnd != IntPtr.Zero)
            {
                _resizeSyncTimer.Stop();
                _attachedChildHwnd = IntPtr.Zero;
                SetFailureStatus($"mdCAD exited after attach (exit code {exitCode}).");
                return;
            }

            string detail = _capturedFailureLine
                ?? $"mdCAD exited before child attach (exit code {exitCode}).";
            SetFailureStatus(detail);
        });
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
            SetFailureStatus("mdCAD process was not started.");
            return;
        }

        if (_mdcadProcess.HasExited)
        {
            int exitCode = _mdcadProcess.ExitCode;
            string detail = _capturedFailureLine
                ?? $"mdCAD exited before child attach (exit code {exitCode}).";
            SetFailureStatus(detail);
            return;
        }

        IntPtr childHwnd = FindChildWindowForProcess(_launchParentHwnd, _mdcadProcess.Id);
        if (childHwnd != IntPtr.Zero)
        {
            _attachedChildHwnd = childHwnd;
            _attachTimer.Stop();
            SyncAttachedChildBounds();
            _resizeSyncTimer.Start();
            _statusTextBlock.Text = StatusAttached;
            _failureTextBlock.Text =
                $"Attached child HWND: 0x{childHwnd.ToInt64():X}{Environment.NewLine}" +
                $"Parent HWND: 0x{_launchParentHwnd.ToInt64():X}";
            if ((_options.TestMode == EmbedTestMode.DestroyedParent ||
                 _options.TestMode == EmbedTestMode.DestroyAfterAttach) &&
                !_destroyAfterAttachTriggered)
            {
                Dispatcher.UIThread.Post(BeginPostAttachTeardown, DispatcherPriority.Background);
            }
            return;
        }

        if ((DateTimeOffset.UtcNow - _launchStartedAt) >= TimeSpan.FromSeconds(10))
        {
            SetFailureStatus(_capturedFailureLine ?? "Timed out waiting for child attach.");
        }
    }

    private void SetFailureStatus(string detail)
    {
        _attachTimer.Stop();
        _launchRetryTimer.Stop();
        _resizeSyncTimer.Stop();
        _statusTextBlock.Text = StatusTimeoutFailure;
        _failureTextBlock.Text = string.IsNullOrWhiteSpace(_capturedFailureLine)
            ? detail
            : $"{_capturedFailureLine}{Environment.NewLine}{detail}";
    }

    private void BeginPostAttachTeardown()
    {
        if (_destroyAfterAttachTriggered || _attachedChildHwnd == IntPtr.Zero || _placeholderHwnd == IntPtr.Zero)
        {
            return;
        }

        _destroyAfterAttachTriggered = true;
        _teardownReason = _options.TestMode == EmbedTestMode.DestroyedParent
            ? "Destroyed-parent mode destroyed the placeholder after attach and is waiting for the mdCAD invalid-parent quit path."
            : "Destroy-after-attach mode destroyed the placeholder after attach and armed host fallback cleanup.";
        _statusTextBlock.Text = StatusTeardownCleanup;
        _failureTextBlock.Text = _teardownReason;

        IntPtr placeholderToDestroy = _placeholderHwnd;
        _placeholderHwnd = IntPtr.Zero;
        _embedSurface.ClearPlaceholderHandle();

        if (placeholderToDestroy != IntPtr.Zero && Win32NativeMethods.IsWindow(placeholderToDestroy))
        {
            Win32NativeMethods.DestroyWindow(placeholderToDestroy);
        }
    }

    private bool EnsureAttachedLifecycleIsHealthy(string origin)
    {
        if (_attachedChildHwnd == IntPtr.Zero)
        {
            return false;
        }

        string? teardownDetail = null;
        if (_placeholderHwnd == IntPtr.Zero)
        {
            teardownDetail = $"{origin}: placeholder HWND no longer exists.";
        }
        else if (!Win32NativeMethods.IsWindow(_placeholderHwnd))
        {
            teardownDetail = $"{origin}: placeholder HWND is invalid.";
        }
        else if (!Win32NativeMethods.IsWindow(_attachedChildHwnd))
        {
            teardownDetail = $"{origin}: attached child HWND is invalid.";
        }
        else if (Win32NativeMethods.GetParent(_attachedChildHwnd) != _placeholderHwnd)
        {
            teardownDetail = $"{origin}: attached child HWND is no longer parented to the placeholder.";
        }

        if (teardownDetail == null)
        {
            return true;
        }

        if (_teardownAwaitingProcessExit)
        {
            return false;
        }

        if (_options.TestMode == EmbedTestMode.DestroyedParent && !_fallbackKillIssued)
        {
            BeginGracefulTeardownWait(teardownDetail, "Waiting for mdCAD self-exit before host fallback cleanup.");
            return false;
        }

        EnsureAttachedProcessTeardown(teardownDetail);
        return false;
    }

    private void BeginGracefulTeardownWait(string detail, string waitingMessage)
    {
        _attachTimer.Stop();
        _launchRetryTimer.Stop();
        _resizeSyncTimer.Stop();
        _teardownReason ??= detail;
        _statusTextBlock.Text = StatusTeardownCleanup;

        if (_mdcadProcess == null || _mdcadProcess.HasExited)
        {
            FinalizeTeardownStatus(_mdcadProcess?.HasExited == true ? _mdcadProcess.ExitCode : 0);
            return;
        }

        if (_teardownAwaitingProcessExit)
        {
            return;
        }

        _teardownAwaitingProcessExit = true;
        _failureTextBlock.Text = $"{_teardownReason}{Environment.NewLine}{waitingMessage}";
        Process process = _mdcadProcess;
        _ = ObserveGracefulExitAsync(process, GracefulEmbeddedExitWait);
    }

    private async Task ObserveGracefulExitAsync(Process process, TimeSpan timeout)
    {
        bool exitedGracefully = await WaitForExitAsync(process, timeout);
        await Dispatcher.UIThread.InvokeAsync(() =>
        {
            if (!_teardownAwaitingProcessExit)
            {
                return;
            }

            _teardownAwaitingProcessExit = false;
            if (_mdcadProcess == null)
            {
                return;
            }

            if (exitedGracefully || _mdcadProcess.HasExited)
            {
                FinalizeTeardownStatus(_mdcadProcess.ExitCode);
                return;
            }

            EnsureAttachedProcessTeardown($"{_teardownReason}{Environment.NewLine}mdCAD did not self-exit within the graceful wait window.");
        });
    }

    private static async Task<bool> WaitForExitAsync(Process process, TimeSpan timeout)
    {
        try
        {
            using CancellationTokenSource cts = new(timeout);
            await process.WaitForExitAsync(cts.Token);
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

    private void WaitForGracefulExitOnClose(string detail)
    {
        _attachTimer.Stop();
        _launchRetryTimer.Stop();
        _resizeSyncTimer.Stop();
        _teardownAwaitingProcessExit = false;
        _teardownReason ??= detail;
        _statusTextBlock.Text = StatusTeardownCleanup;

        if (_mdcadProcess == null)
        {
            return;
        }

        if (_mdcadProcess.HasExited)
        {
            FinalizeTeardownStatus(_mdcadProcess.ExitCode);
            return;
        }

        _failureTextBlock.Text = $"{_teardownReason}{Environment.NewLine}"
            + "Waiting for mdCAD to exit so the embedded layout can flush before fallback cleanup.";

        bool exitedGracefully;
        try
        {
            exitedGracefully = _mdcadProcess.WaitForExit((int)GracefulEmbeddedExitWait.TotalMilliseconds);
        }
        catch (InvalidOperationException)
        {
            exitedGracefully = _mdcadProcess.HasExited;
        }

        if (exitedGracefully || _mdcadProcess.HasExited)
        {
            FinalizeTeardownStatus(_mdcadProcess.ExitCode);
            return;
        }

        EnsureAttachedProcessTeardown($"{_teardownReason}{Environment.NewLine}mdCAD did not exit before the host-close timeout.");
    }

    private void EnsureAttachedProcessTeardown(string detail)
    {
        _attachTimer.Stop();
        _launchRetryTimer.Stop();
        _resizeSyncTimer.Stop();
        _teardownAwaitingProcessExit = false;
        _teardownReason ??= detail;
        _statusTextBlock.Text = StatusTeardownCleanup;

        if (_mdcadProcess == null || _mdcadProcess.HasExited)
        {
            FinalizeTeardownStatus(_mdcadProcess?.HasExited == true ? _mdcadProcess.ExitCode : 0);
            return;
        }

        try
        {
            _fallbackKillIssued = true;
            _mdcadProcess.Kill(true);
        }
        catch (InvalidOperationException)
        {
            FinalizeTeardownStatus(_mdcadProcess?.HasExited == true ? _mdcadProcess.ExitCode : -1);
        }
    }

    private void FinalizeTeardownStatus(int exitCode)
    {
        _attachTimer.Stop();
        _launchRetryTimer.Stop();
        _resizeSyncTimer.Stop();
        _teardownAwaitingProcessExit = false;
        _attachedChildHwnd = IntPtr.Zero;
        _statusTextBlock.Text = StatusTeardownCleanup;

        string exitSummary = _fallbackKillIssued
            ? $"Host fallback cleanup completed (exit code {exitCode}); no surviving mdCAD.exe."
            : $"mdCAD self-exit observed (exit code {exitCode}); no surviving mdCAD.exe.";
        _failureTextBlock.Text = string.IsNullOrWhiteSpace(_teardownReason)
            ? exitSummary
            : $"{_teardownReason}{Environment.NewLine}{exitSummary}";
    }

    private bool TryResolveMdcadExecutable(out string? mdCadExePath, out string failureMessage)
    {
        if (!string.IsNullOrWhiteSpace(_options.MdcadExePath))
        {
            string overridePath = Path.GetFullPath(_options.MdcadExePath);
            if (File.Exists(overridePath))
            {
                mdCadExePath = overridePath;
                failureMessage = string.Empty;
                return true;
            }

            mdCadExePath = null;
            failureMessage = $"Unable to resolve mdCAD.exe override: {overridePath}";
            return false;
        }

        string repoRoot = FindRepoRoot();
        string[] candidates =
        {
            Path.Combine(repoRoot, "build-vulkan", "bin", "Release", "mdCAD.exe"),
            Path.Combine(repoRoot, "build-vulkan", "bin", "mdCAD.exe"),
        };

        foreach (string candidate in candidates)
        {
            if (File.Exists(candidate))
            {
                mdCadExePath = candidate;
                failureMessage = string.Empty;
                return true;
            }
        }

        mdCadExePath = null;
        failureMessage =
            $"Unable to resolve mdCAD.exe. Checked:{Environment.NewLine}" +
            string.Join(Environment.NewLine, candidates);
        return false;
    }

    private static string FindRepoRoot()
    {
        DirectoryInfo? current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current != null)
        {
            bool hasPlanning = Directory.Exists(Path.Combine(current.FullName, ".planning"));
            bool hasSource = Directory.Exists(Path.Combine(current.FullName, "src"));
            if (hasPlanning && hasSource)
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        return Directory.GetCurrentDirectory();
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

    private void OnEmbedSurfaceSizeChanged(object? sender, SizeChangedEventArgs e)
    {
        SyncAttachedChildBounds();
    }

    private void OnHostChromePointerPressed(object? sender, PointerPressedEventArgs e)
    {
        if (sender is Control control)
        {
            control.Focus();
            e.Handled = true;
            return;
        }

        Focus();
    }

    private void SyncAttachedChildBounds()
    {
        if (_attachedChildHwnd == IntPtr.Zero)
        {
            return;
        }

        if (!EnsureAttachedLifecycleIsHealthy("Resize sync"))
        {
            return;
        }

        if (!Win32NativeMethods.TryGetClientSize(_placeholderHwnd, out int width, out int height))
        {
            return;
        }

        if (width <= 0 || height <= 0)
        {
            return;
        }

        Win32NativeMethods.MoveWindow(_attachedChildHwnd, 0, 0, width, height, true);
    }

    private void OnResizeSyncTick(object? sender, EventArgs e)
    {
        if (_attachedChildHwnd == IntPtr.Zero)
        {
            _resizeSyncTimer.Stop();
            return;
        }

        if (!EnsureAttachedLifecycleIsHealthy("Resize monitor"))
        {
            return;
        }

        SyncAttachedChildBounds();
    }

    private void OnWindowClosed(object? sender, EventArgs e)
    {
        WaitForGracefulExitOnClose("Host window closed; embedded mdCAD must not remain orphaned.");

        if (_mdcadProcess != null)
        {
            try
            {
                if (!_mdcadProcess.HasExited)
                {
                    _mdcadProcess.Kill(true);
                }
            }
            catch (InvalidOperationException)
            {
            }

            _mdcadProcess.Dispose();
            _mdcadProcess = null;
        }
    }
}

internal static class Win32NativeMethods
{
    private const uint WsChild = 0x40000000;
    private const uint WsVisible = 0x10000000;
    private const uint WsClipSiblings = 0x04000000;
    private const uint WsClipChildren = 0x02000000;

    public static IntPtr CreatePlaceholderWindow(IntPtr parentHwnd)
    {
        TryGetClientSize(parentHwnd, out int width, out int height);
        if (width <= 0)
        {
            width = 640;
        }
        if (height <= 0)
        {
            height = 480;
        }

        IntPtr hwnd = CreateWindowExW(
            0,
            "STATIC",
            string.Empty,
            WsChild | WsVisible | WsClipSiblings | WsClipChildren,
            0,
            0,
            width,
            height,
            parentHwnd,
            IntPtr.Zero,
            IntPtr.Zero,
            IntPtr.Zero);

        if (hwnd == IntPtr.Zero)
        {
            throw new InvalidOperationException("Failed to create a placeholder HWND for NativeControlHost.");
        }

        return hwnd;
    }

    public static bool TryGetClientSize(IntPtr hwnd, out int width, out int height)
    {
        width = 0;
        height = 0;
        if (hwnd == IntPtr.Zero)
        {
            return false;
        }

        if (!GetClientRect(hwnd, out RECT rect))
        {
            return false;
        }

        width = rect.Right - rect.Left;
        height = rect.Bottom - rect.Top;
        return true;
    }

    public delegate bool EnumWindowsProc(IntPtr hwnd, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr CreateWindowExW(
        uint dwExStyle,
        string lpClassName,
        string lpWindowName,
        uint dwStyle,
        int x,
        int y,
        int nWidth,
        int nHeight,
        IntPtr hWndParent,
        IntPtr hMenu,
        IntPtr hInstance,
        IntPtr lpParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool DestroyWindow(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsWindow(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr GetParent(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int nWidth, int nHeight, [MarshalAs(UnmanagedType.Bool)] bool bRepaint);

    [StructLayout(LayoutKind.Sequential)]
    private struct RECT
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
}
