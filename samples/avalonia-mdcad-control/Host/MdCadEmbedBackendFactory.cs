using MdCad.Avalonia.Control.Host.Windows;

namespace MdCad.Avalonia.Control.Host;

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
}
