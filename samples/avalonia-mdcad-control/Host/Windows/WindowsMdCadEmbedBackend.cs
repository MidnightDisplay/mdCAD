using System.ComponentModel;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;

using Avalonia;
using Avalonia.Controls;
using Avalonia.Threading;
using AvaloniaControl = Avalonia.Controls.Control;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control.Host.Windows;

internal sealed class WindowsMdCadEmbedBackend : IMdCadEmbedBackend
{
    private static readonly TimeSpan GracefulEmbeddedExitWait = TimeSpan.FromSeconds(3);
    private static readonly TimeSpan ChildAttachTimeout = TimeSpan.FromSeconds(10);

    private readonly Action<string> _setLaunchStatus;
    private readonly Action<string> _setAttachStatus;
    private readonly Action<MdCadLaunchSnapshot> _updatePreLaunchStatus;
    private readonly Action<MdCadLaunchSnapshot> _updatePostLaunchStatus;
    private readonly Action<string> _setFailureStatus;
    private readonly Action<string?> _setLaunchWarning;
    private readonly Action _updateControlState;
    private readonly Func<MdCadLaunchSnapshot> _captureLaunchSnapshot;
    private readonly DispatcherTimer _attachTimer;
    private readonly DispatcherTimer _resizeSyncTimer;

    private EmbedNativeControlHost _surface;
    private Process? _mdcadProcess;
    private IntPtr _placeholderHandle;
    private IntPtr _launchParentHwnd;
    private IntPtr _attachedChildHwnd;
    private DateTimeOffset _launchStartedAt;
    private string? _capturedFailureLine;
    private MdCadLaunchSnapshot _lastLaunchSnapshot;

    public WindowsMdCadEmbedBackend(
        MdCadLaunchSnapshot initialSnapshot,
        Action<string> setLaunchStatus,
        Action<string> setAttachStatus,
        Action<MdCadLaunchSnapshot> updatePreLaunchStatus,
        Action<MdCadLaunchSnapshot> updatePostLaunchStatus,
        Action<string> setFailureStatus,
        Action<string?> setLaunchWarning,
        Action updateControlState,
        Func<MdCadLaunchSnapshot> captureLaunchSnapshot)
    {
        _lastLaunchSnapshot = initialSnapshot;
        _setLaunchStatus = setLaunchStatus ?? throw new ArgumentNullException(nameof(setLaunchStatus));
        _setAttachStatus = setAttachStatus ?? throw new ArgumentNullException(nameof(setAttachStatus));
        _updatePreLaunchStatus = updatePreLaunchStatus ?? throw new ArgumentNullException(nameof(updatePreLaunchStatus));
        _updatePostLaunchStatus = updatePostLaunchStatus ?? throw new ArgumentNullException(nameof(updatePostLaunchStatus));
        _setFailureStatus = setFailureStatus ?? throw new ArgumentNullException(nameof(setFailureStatus));
        _setLaunchWarning = setLaunchWarning ?? throw new ArgumentNullException(nameof(setLaunchWarning));
        _updateControlState = updateControlState ?? throw new ArgumentNullException(nameof(updateControlState));
        _captureLaunchSnapshot = captureLaunchSnapshot ?? throw new ArgumentNullException(nameof(captureLaunchSnapshot));

        _attachTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };
        _resizeSyncTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(33) };
        _attachTimer.Tick += OnAttachTimerTick;
        _resizeSyncTimer.Tick += OnResizeSyncTick;
        _surface = CreateSurface();
    }

    public AvaloniaControl Surface => _surface;

    public bool CanStartSession =>
        _placeholderHandle != IntPtr.Zero &&
        Win32NativeMethods.TryGetClientSize(_placeholderHandle, out int width, out int height) &&
        width > 4 &&
        height > 4;

    public string? StartBlockedReason => null;

    public IntPtr PlaceholderHandle => _placeholderHandle;

    public bool HasActiveSession => _mdcadProcess != null || _attachedChildHwnd != IntPtr.Zero;

    public event Action? StateChanged;

    public event Action<string>? UnexpectedSessionLoss;

    public async Task StartSessionAsync(MdCadLaunchSnapshot snapshot, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        if (_mdcadProcess != null)
        {
            throw new InvalidOperationException("An mdCAD session is already running.");
        }

        if (_placeholderHandle == IntPtr.Zero)
        {
            throw new InvalidOperationException("Embedded placeholder HWND is not ready.");
        }

        MdCadRuntimePaths runtime = MdCadRuntimeResolver.Resolve();
        _lastLaunchSnapshot = snapshot;
        _setLaunchStatus("launching");
        _setAttachStatus("waiting for child attach");
        _updatePreLaunchStatus(snapshot);
        _setFailureStatus($"runtime: {runtime.ExecutablePath}; parent hwnd: 0x{_placeholderHandle.ToInt64():X}");
        _updateControlState();

        Process process = new()
        {
            StartInfo = CreateStartInfo(runtime, _placeholderHandle, snapshot),
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
        _launchParentHwnd = _placeholderHandle;
        _launchStartedAt = DateTimeOffset.UtcNow;
        _mdcadProcess.BeginOutputReadLine();
        _mdcadProcess.BeginErrorReadLine();
        _attachTimer.Start();
        _updateControlState();
    }

    public async Task StopSessionAsync(CancellationToken cancellationToken)
    {
        _attachTimer.Stop();
        _resizeSyncTimer.Stop();

        bool hadSession = _mdcadProcess != null || _attachedChildHwnd != IntPtr.Zero;
        if (hadSession)
        {
            _setLaunchStatus("teardown/cleanup");
            _setFailureStatus("Host requested stop/close.");
            _updateControlState();
        }

        IntPtr placeholderToDestroy = _placeholderHandle;
        _placeholderHandle = IntPtr.Zero;
        _surface.ClearPlaceholderHandle();
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
        _setLaunchStatus("idle");
        _setAttachStatus("idle");
        _updatePreLaunchStatus(_captureLaunchSnapshot());
        _updateControlState();
    }

    public Task RecreateSurfaceAsync(CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();

        _surface.PlaceholderHandleReady -= OnPlaceholderHandleReady;
        _surface.SizeChanged -= OnSurfaceSizeChanged;
        _placeholderHandle = IntPtr.Zero;
        _surface = CreateSurface();
        _setAttachStatus("idle");
        _updateControlState();
        StateChanged?.Invoke();
        return Task.CompletedTask;
    }

    private EmbedNativeControlHost CreateSurface()
    {
        EmbedNativeControlHost host = new();
        host.PlaceholderHandleReady += OnPlaceholderHandleReady;
        host.SizeChanged += OnSurfaceSizeChanged;
        return host;
    }

    private void OnPlaceholderHandleReady(IntPtr hwnd)
    {
        _placeholderHandle = hwnd;
        _updateControlState();
        StateChanged?.Invoke();
    }

    private void OnSurfaceSizeChanged(object? sender, SizeChangedEventArgs e)
    {
        if (_attachedChildHwnd != IntPtr.Zero)
        {
            SyncAttachedChildBounds();
        }

        _updateControlState();
        StateChanged?.Invoke();
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
            _setLaunchStatus("active");
            _setAttachStatus("attached");
            _updatePostLaunchStatus(_lastLaunchSnapshot);
            _setFailureStatus($"child hwnd attached: 0x{childHwnd.ToInt64():X}");
            _updateControlState();
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
                    _setLaunchWarning(_capturedFailureLine);
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
        _setLaunchStatus("timeout/failure");
        _setAttachStatus("idle");
        _updatePreLaunchStatus(_captureLaunchSnapshot());
        _updateControlState();
        UnexpectedSessionLoss?.Invoke(detail);
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

    internal static ProcessStartInfo CreateStartInfo(
        MdCadRuntimePaths runtime,
        IntPtr placeholderHandle,
        MdCadLaunchSnapshot snapshot)
    {
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

        if (snapshot.ViewportOnlyStartupMode)
        {
            startInfo.ArgumentList.Add("--viewport-only");
        }

        if (snapshot.ShouldPassJsonlArgument)
        {
            startInfo.ArgumentList.Add("--jsonl");
            startInfo.ArgumentList.Add(snapshot.LaunchJsonlPath!);
        }

        if (snapshot.ShouldPassLiveRefreshArgument)
        {
            startInfo.ArgumentList.Add("--jsonl-live-refresh");
        }

        return startInfo;
    }
}
