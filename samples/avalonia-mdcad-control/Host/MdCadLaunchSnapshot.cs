using System.IO;

namespace MdCad.Avalonia.Control.Host;

internal readonly record struct MdCadLaunchSnapshot(
    string? RequestedJsonlPath,
    string? LaunchJsonlPath,
    bool StartupLiveRefreshEnabled,
    bool ViewportOnlyStartupMode,
    string? WarningText)
{
    public bool ShouldPassJsonlArgument => !string.IsNullOrWhiteSpace(LaunchJsonlPath);

    public bool ShouldPassLiveRefreshArgument => ShouldPassJsonlArgument && StartupLiveRefreshEnabled;

    public static MdCadLaunchSnapshot Create(
        string? requestedJsonlPath,
        bool startupLiveRefreshEnabled,
        bool viewportOnlyStartupMode)
    {
        string? normalizedRequest = NormalizeRequestedJsonlPath(requestedJsonlPath);
        if (normalizedRequest == null)
        {
            return new MdCadLaunchSnapshot(
                RequestedJsonlPath: null,
                LaunchJsonlPath: null,
                StartupLiveRefreshEnabled: startupLiveRefreshEnabled,
                ViewportOnlyStartupMode: viewportOnlyStartupMode,
                WarningText: null);
        }

        if (!IsNativeAbsolutePath(normalizedRequest))
        {
            return new MdCadLaunchSnapshot(
                RequestedJsonlPath: normalizedRequest,
                LaunchJsonlPath: null,
                StartupLiveRefreshEnabled: startupLiveRefreshEnabled,
                ViewportOnlyStartupMode: viewportOnlyStartupMode,
                WarningText: $"Requested JSONL path must be absolute: {normalizedRequest}");
        }

        string? warningText = CanReadFile(normalizedRequest)
            ? null
            : $"Requested JSONL file is missing or unreadable: {normalizedRequest}";

        return new MdCadLaunchSnapshot(
            RequestedJsonlPath: normalizedRequest,
            LaunchJsonlPath: normalizedRequest,
            StartupLiveRefreshEnabled: startupLiveRefreshEnabled,
            ViewportOnlyStartupMode: viewportOnlyStartupMode,
            WarningText: warningText);
    }

    private static string? NormalizeRequestedJsonlPath(string? requestedJsonlPath)
    {
        if (string.IsNullOrWhiteSpace(requestedJsonlPath))
        {
            return null;
        }

        return requestedJsonlPath.Trim();
    }

    private static bool IsNativeAbsolutePath(string path)
    {
        if (path.Length <= 2)
        {
            return false;
        }

        if (path[0] == '\\' && path[1] == '\\')
        {
            return true;
        }

        return path[1] == ':' && (path[2] == '\\' || path[2] == '/');
    }

    private static bool CanReadFile(string path)
    {
        try
        {
            using FileStream stream = new(
                path,
                FileMode.Open,
                FileAccess.Read,
                FileShare.ReadWrite | FileShare.Delete);
            return true;
        }
        catch (IOException)
        {
            return false;
        }
        catch (UnauthorizedAccessException)
        {
            return false;
        }
    }
}
