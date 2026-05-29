using System.Diagnostics;
using System.IO;

using MdCad.Embed.Core.Windows;

namespace MdCad.Embed.Core.Tests;

public sealed class MdCadWindowsStartInfoBuilderTests
{
    [Fact]
    public void CreateEmbeddedProcessStartInfo_UsesRuntimeExecutableWorkingDirectoryAndRequiredArgs()
    {
        ProcessStartInfo startInfo = MdCadWindowsStartInfoBuilder.CreateEmbeddedProcessStartInfo(
            executablePath: @"C:\runtime\mdCAD.exe",
            runtimeRoot: @"C:\runtime",
            parentHwnd: new IntPtr(0x1234),
            viewportOnlyStartupMode: false,
            launchJsonlPath: null,
            startupLiveRefreshEnabled: true);

        Assert.Equal(@"C:\runtime\mdCAD.exe", startInfo.FileName);
        Assert.Equal(@"C:\runtime", startInfo.WorkingDirectory);
        Assert.False(startInfo.UseShellExecute);
        Assert.True(startInfo.RedirectStandardOutput);
        Assert.True(startInfo.RedirectStandardError);
        Assert.False(startInfo.CreateNoWindow);
        Assert.Equal(["--embedded", "--parent-hwnd", "0x1234"], startInfo.ArgumentList);
    }

    [Fact]
    public void CreateEmbeddedProcessStartInfo_ForwardsViewportOnlyJsonlAndLiveRefreshInLockedOrder()
    {
        string jsonlPath = CreateReadableJsonlPath();
        try
        {
            ProcessStartInfo startInfo = MdCadWindowsStartInfoBuilder.CreateEmbeddedProcessStartInfo(
                executablePath: @"C:\runtime\mdCAD.exe",
                runtimeRoot: @"C:\runtime",
                parentHwnd: new IntPtr(0xCAFE),
                viewportOnlyStartupMode: true,
                launchJsonlPath: jsonlPath,
                startupLiveRefreshEnabled: true);

            Assert.Equal(
                ["--embedded", "--parent-hwnd", "0xCAFE", "--viewport-only", "--jsonl", jsonlPath, "--jsonl-live-refresh"],
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
