using System.Threading;

namespace MdCad.Embed.Core;

public sealed class MdCadSessionCoordinator : IAsyncDisposable
{
    private readonly Func<MdCadLaunchSnapshot> _captureSnapshot;
    private readonly Func<bool> _canStartSession;
    private readonly Func<IntPtr> _getPlaceholderHandle;
    private readonly Func<bool> _getAutoStart;
    private readonly Func<string?> _getStartBlockedReason;
    private readonly Action<string?> _applyWarning;
    private readonly Func<MdCadLaunchSnapshot, CancellationToken, Task> _startSessionAsync;
    private readonly Func<CancellationToken, Task> _stopSessionAsync;
    private readonly Func<CancellationToken, Task> _recreateSurfaceAsync;
    private readonly SemaphoreSlim _lifecycleGate = new(1, 1);

    private long _requestedGeneration;
    private bool _autoStartSuppressed;
    private bool _desiredRunning;
    private bool _hasStartedOnce;
    private bool _isDisposed;
    private bool _isSessionRunning;
    private MdCadLaunchSnapshot? _activeSnapshot;

    public MdCadSessionCoordinator(
        Func<MdCadLaunchSnapshot> captureSnapshot,
        Func<bool> canStartSession,
        Func<IntPtr> getPlaceholderHandle,
        Func<bool> getAutoStart,
        Action<string?> applyWarning,
        Func<MdCadLaunchSnapshot, CancellationToken, Task> startSessionAsync,
        Func<CancellationToken, Task> stopSessionAsync,
        Func<CancellationToken, Task> recreateSurfaceAsync,
        Func<string?>? getStartBlockedReason = null)
    {
        _captureSnapshot = captureSnapshot ?? throw new ArgumentNullException(nameof(captureSnapshot));
        _canStartSession = canStartSession ?? throw new ArgumentNullException(nameof(canStartSession));
        _getPlaceholderHandle = getPlaceholderHandle ?? throw new ArgumentNullException(nameof(getPlaceholderHandle));
        _getAutoStart = getAutoStart ?? throw new ArgumentNullException(nameof(getAutoStart));
        _getStartBlockedReason = getStartBlockedReason ?? (() => null);
        _applyWarning = applyWarning ?? throw new ArgumentNullException(nameof(applyWarning));
        _startSessionAsync = startSessionAsync ?? throw new ArgumentNullException(nameof(startSessionAsync));
        _stopSessionAsync = stopSessionAsync ?? throw new ArgumentNullException(nameof(stopSessionAsync));
        _recreateSurfaceAsync = recreateSurfaceAsync ?? throw new ArgumentNullException(nameof(recreateSurfaceAsync));
    }

    public bool IsSessionRunning => _isSessionRunning;

    public Task RequestReconcileAsync()
    {
        return QueueReconcileAsync(Interlocked.Increment(ref _requestedGeneration));
    }

    public Task StartAsync()
    {
        _desiredRunning = true;
        _autoStartSuppressed = false;
        string? startBlockedReason = GetStartBlockedReason();
        if (startBlockedReason != null)
        {
            _applyWarning(startBlockedReason);
            return Task.FromException(new PlatformNotSupportedException(startBlockedReason));
        }

        return ReconcileAsync(Interlocked.Increment(ref _requestedGeneration));
    }

    public Task StopAsync()
    {
        _desiredRunning = false;
        _autoStartSuppressed = true;
        return ReconcileAsync(Interlocked.Increment(ref _requestedGeneration));
    }

    public async ValueTask DisposeAsync()
    {
        if (_isDisposed)
        {
            return;
        }

        _desiredRunning = false;
        _autoStartSuppressed = true;
        await ReconcileAsync(Interlocked.Increment(ref _requestedGeneration));
        _isDisposed = true;
        _lifecycleGate.Dispose();
    }

    private async Task ReconcileAsync(long generation)
    {
        await _lifecycleGate.WaitAsync();
        try
        {
            if (_isDisposed || generation != _requestedGeneration)
            {
                return;
            }

            MdCadLaunchSnapshot snapshot = _captureSnapshot();
            string? startBlockedReason = GetStartBlockedReason();
            _applyWarning(startBlockedReason ?? snapshot.WarningText);

            bool shouldBeRunning = ShouldBeRunning(startBlockedReason);
            bool launchChanged = _isSessionRunning && (!_activeSnapshot.HasValue || _activeSnapshot.Value != snapshot);

            if (_isSessionRunning && (!shouldBeRunning || launchChanged))
            {
                await _stopSessionAsync(CancellationToken.None);
                _isSessionRunning = false;
                _activeSnapshot = null;
                await _recreateSurfaceAsync(CancellationToken.None);

                if (_isDisposed || generation != _requestedGeneration)
                {
                    return;
                }

                snapshot = _captureSnapshot();
                startBlockedReason = GetStartBlockedReason();
                _applyWarning(startBlockedReason ?? snapshot.WarningText);
                shouldBeRunning = ShouldBeRunning(startBlockedReason);
            }

            if (_isDisposed || generation != _requestedGeneration || _isSessionRunning || !shouldBeRunning)
            {
                return;
            }

            IntPtr placeholderHandle = _getPlaceholderHandle();
            if (placeholderHandle == IntPtr.Zero)
            {
                return;
            }

            await _startSessionAsync(snapshot, CancellationToken.None);
            _isSessionRunning = true;
            _activeSnapshot = snapshot;
            _desiredRunning = true;
            _hasStartedOnce = true;
        }
        finally
        {
            _lifecycleGate.Release();
        }
    }

    private bool ShouldBeRunning(string? startBlockedReason)
    {
        if (startBlockedReason != null)
        {
            return false;
        }

        if (!_canStartSession())
        {
            return false;
        }

        if (_desiredRunning)
        {
            return true;
        }

        return !_hasStartedOnce && !_autoStartSuppressed && _getAutoStart();
    }

    private string? GetStartBlockedReason()
    {
        string? startBlockedReason = _getStartBlockedReason();
        return string.IsNullOrWhiteSpace(startBlockedReason) ? null : startBlockedReason;
    }

    private async Task QueueReconcileAsync(long generation)
    {
        await Task.Yield();
        await ReconcileAsync(generation);
    }
}
