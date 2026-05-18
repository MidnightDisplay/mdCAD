using System.Diagnostics;
using System.IO;

using MdCad.Avalonia.Control.Host;
using MdCad.Avalonia.Control.Host.Windows;

namespace MdCad.Avalonia.Control.Tests;

public sealed class WindowsMdCadEmbedBackendTests
{
    [Fact]
    public void CreateStartInfo_UsesRuntimeExecutableWorkingDirectoryAndRequiredArgs()
    {
        MdCadRuntimePaths runtime = new(
            ExecutablePath: @"C:\runtime\mdCAD.exe",
            RuntimeRoot: @"C:\runtime");

        ProcessStartInfo startInfo = WindowsMdCadEmbedBackend.CreateStartInfo(
            runtime,
            new IntPtr(0x1234),
            MdCadLaunchSnapshot.Create(null, startupLiveRefreshEnabled: true));

        Assert.Equal(runtime.ExecutablePath, startInfo.FileName);
        Assert.Equal(runtime.RuntimeRoot, startInfo.WorkingDirectory);
        Assert.False(startInfo.UseShellExecute);
        Assert.True(startInfo.RedirectStandardOutput);
        Assert.True(startInfo.RedirectStandardError);
        Assert.False(startInfo.CreateNoWindow);
        Assert.Equal(["--embedded", "--parent-hwnd", "0x1234"], startInfo.ArgumentList);
    }

    [Fact]
    public void CreateStartInfo_ForwardsJsonlWithoutLiveRefreshWhenSnapshotDisablesIt()
    {
        string jsonlPath = CreateReadableJsonlPath();
        try
        {
            MdCadRuntimePaths runtime = new(
                ExecutablePath: @"C:\runtime\mdCAD.exe",
                RuntimeRoot: @"C:\runtime");

            ProcessStartInfo startInfo = WindowsMdCadEmbedBackend.CreateStartInfo(
                runtime,
                new IntPtr(0xFEED),
                MdCadLaunchSnapshot.Create(jsonlPath, startupLiveRefreshEnabled: false));

            Assert.Equal(
                ["--embedded", "--parent-hwnd", "0xFEED", "--jsonl", jsonlPath],
                startInfo.ArgumentList);
        }
        finally
        {
            File.Delete(jsonlPath);
        }
    }

    [Fact]
    public void CreateStartInfo_ForwardsJsonlAndLiveRefreshWhenSnapshotEnablesIt()
    {
        string jsonlPath = CreateReadableJsonlPath();
        try
        {
            MdCadRuntimePaths runtime = new(
                ExecutablePath: @"C:\runtime\mdCAD.exe",
                RuntimeRoot: @"C:\runtime");

            ProcessStartInfo startInfo = WindowsMdCadEmbedBackend.CreateStartInfo(
                runtime,
                new IntPtr(0xBEEF),
                MdCadLaunchSnapshot.Create(jsonlPath, startupLiveRefreshEnabled: true));

            Assert.Equal(
                ["--embedded", "--parent-hwnd", "0xBEEF", "--jsonl", jsonlPath, "--jsonl-live-refresh"],
                startInfo.ArgumentList);
        }
        finally
        {
            File.Delete(jsonlPath);
        }
    }

    private static string CreateReadableJsonlPath()
    {
        string path = Path.Combine(Path.GetTempPath(), $"mdcad-launch-{Guid.NewGuid():N}.jsonl");
        File.WriteAllText(path, "{}");
        return path;
    }
}
