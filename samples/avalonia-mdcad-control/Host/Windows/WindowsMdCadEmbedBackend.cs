using System.Diagnostics;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control.Host.Windows;

internal sealed class WindowsMdCadEmbedBackend
{
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
