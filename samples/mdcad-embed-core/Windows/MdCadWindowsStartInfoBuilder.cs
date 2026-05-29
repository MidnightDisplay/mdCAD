using System.Diagnostics;

namespace MdCad.Embed.Core.Windows;

public static class MdCadWindowsStartInfoBuilder
{
    public static ProcessStartInfo CreateEmbeddedProcessStartInfo(
        string executablePath,
        string runtimeRoot,
        IntPtr parentHwnd,
        bool viewportOnlyStartupMode,
        string? launchJsonlPath,
        bool startupLiveRefreshEnabled)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(executablePath);
        ArgumentException.ThrowIfNullOrWhiteSpace(runtimeRoot);

        ProcessStartInfo startInfo = new()
        {
            FileName = executablePath,
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = false,
            WorkingDirectory = runtimeRoot,
        };

        startInfo.ArgumentList.Add("--embedded");
        startInfo.ArgumentList.Add("--parent-hwnd");
        startInfo.ArgumentList.Add($"0x{parentHwnd.ToInt64():X}");

        if (viewportOnlyStartupMode)
        {
            startInfo.ArgumentList.Add("--viewport-only");
        }

        if (!string.IsNullOrWhiteSpace(launchJsonlPath))
        {
            startInfo.ArgumentList.Add("--jsonl");
            startInfo.ArgumentList.Add(launchJsonlPath);

            if (startupLiveRefreshEnabled)
            {
                startInfo.ArgumentList.Add("--jsonl-live-refresh");
            }
        }

        return startInfo;
    }
}
