using System.Threading;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;

using MdCad.Embed.Core;
using MdCad.Wpf.Control.Host.Windows;

namespace MdCad.Wpf.Control.Host;

internal static class MdCadEmbedBackendFactory
{
    internal static IMdCadEmbedBackend Create(
        MdCadLaunchSnapshot initialSnapshot,
        Action<string> setLaunchStatus,
        Action<string> setAttachStatus,
        Action<MdCadLaunchSnapshot> updatePreLaunchStatus,
        Action<MdCadLaunchSnapshot> updatePostLaunchStatus,
        Action<string> setFailureStatus,
        Action<string?> setLaunchWarning,
        Action updateControlState,
        Func<MdCadLaunchSnapshot> captureLaunchSnapshot,
        Func<bool>? isWindows = null)
    {
        Func<bool> platformProbe = isWindows ?? OperatingSystem.IsWindows;
        if (!platformProbe())
        {
            return new UnsupportedMdCadEmbedBackend();
        }

        return new WindowsMdCadEmbedBackend(
            initialSnapshot: initialSnapshot,
            setLaunchStatus: setLaunchStatus,
            setAttachStatus: setAttachStatus,
            updatePreLaunchStatus: updatePreLaunchStatus,
            updatePostLaunchStatus: updatePostLaunchStatus,
            setFailureStatus: setFailureStatus,
            setLaunchWarning: setLaunchWarning,
            updateControlState: updateControlState,
            captureLaunchSnapshot: captureLaunchSnapshot);
    }

    private sealed class UnsupportedMdCadEmbedBackend : IMdCadEmbedBackend
    {
        private readonly Border _surface = new();

        public FrameworkElement Surface => _surface;

        public bool CanStartSession => false;

        public string? StartBlockedReason => MdCadUnsupportedRuntime.UnsupportedRuntimeMessage;

        public IntPtr PlaceholderHandle => IntPtr.Zero;

        public bool HasActiveSession => false;

        public event Action? StateChanged
        {
            add { }
            remove { }
        }

        public event Action<string>? UnexpectedSessionLoss
        {
            add { }
            remove { }
        }

        public Task StartSessionAsync(MdCadLaunchSnapshot snapshot, CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            throw new PlatformNotSupportedException(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage);
        }

        public Task StopSessionAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.CompletedTask;
        }

        public Task RecreateSurfaceAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            return Task.CompletedTask;
        }
    }
}
