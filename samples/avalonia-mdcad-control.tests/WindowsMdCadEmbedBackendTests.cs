using System.Diagnostics;
using System.IO;
using System.Reflection;

using MdCad.Avalonia.Control.Host;
using MdCad.Avalonia.Control.Host.Windows;
using MdCad.Embed.Core.Windows;

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
            MdCadLaunchSnapshot.Create(null, startupLiveRefreshEnabled: true, viewportOnlyStartupMode: false));

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
                MdCadLaunchSnapshot.Create(jsonlPath, startupLiveRefreshEnabled: false, viewportOnlyStartupMode: false));

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
                MdCadLaunchSnapshot.Create(jsonlPath, startupLiveRefreshEnabled: true, viewportOnlyStartupMode: false));

            Assert.Equal(
                ["--embedded", "--parent-hwnd", "0xBEEF", "--jsonl", jsonlPath, "--jsonl-live-refresh"],
                startInfo.ArgumentList);
        }
        finally
        {
            File.Delete(jsonlPath);
        }
    }

    [Fact]
    public void CreateStartInfo_ForwardsViewportOnlyBeforeOptionalJsonlArgs()
    {
        string jsonlPath = CreateReadableJsonlPath();
        try
        {
            MdCadRuntimePaths runtime = new(
                ExecutablePath: @"C:\runtime\mdCAD.exe",
                RuntimeRoot: @"C:\runtime");

            ProcessStartInfo startInfo = WindowsMdCadEmbedBackend.CreateStartInfo(
                runtime,
                new IntPtr(0xCAFE),
                MdCadLaunchSnapshot.Create(jsonlPath, startupLiveRefreshEnabled: true, viewportOnlyStartupMode: true));

            Assert.Equal(
                ["--embedded", "--parent-hwnd", "0xCAFE", "--viewport-only", "--jsonl", jsonlPath, "--jsonl-live-refresh"],
                startInfo.ArgumentList);
        }
        finally
        {
            File.Delete(jsonlPath);
        }
    }

    [Fact]
    public void CreateStartInfo_UsesCurrentPlaceholderHandleOnRelaunch()
    {
        string jsonlPath = CreateReadableJsonlPath();
        try
        {
            MdCadRuntimePaths runtime = new(
                ExecutablePath: @"C:\runtime\mdCAD.exe",
                RuntimeRoot: @"C:\runtime");

            ProcessStartInfo initialStartInfo = WindowsMdCadEmbedBackend.CreateStartInfo(
                runtime,
                new IntPtr(0x1234),
                MdCadLaunchSnapshot.Create(jsonlPath, startupLiveRefreshEnabled: false, viewportOnlyStartupMode: false));

            ProcessStartInfo relaunchedStartInfo = WindowsMdCadEmbedBackend.CreateStartInfo(
                runtime,
                new IntPtr(0x5678),
                MdCadLaunchSnapshot.Create(jsonlPath, startupLiveRefreshEnabled: true, viewportOnlyStartupMode: false));

            Assert.Equal(["--embedded", "--parent-hwnd", "0x1234", "--jsonl", jsonlPath], initialStartInfo.ArgumentList);
            Assert.Equal(
                ["--embedded", "--parent-hwnd", "0x5678", "--jsonl", jsonlPath, "--jsonl-live-refresh"],
                relaunchedStartInfo.ArgumentList);
            Assert.DoesNotContain("0x1234", relaunchedStartInfo.ArgumentList);
            Assert.Equal(runtime.RuntimeRoot, relaunchedStartInfo.WorkingDirectory);
            Assert.Equal(runtime.ExecutablePath, relaunchedStartInfo.FileName);
        }
        finally
        {
            File.Delete(jsonlPath);
        }
    }

    [Fact]
    public void CreateStartInfo_DelegatesToSharedWindowsStartInfoBuilder()
    {
        string jsonlPath = CreateReadableJsonlPath();
        try
        {
            MdCadRuntimePaths runtime = new(
                ExecutablePath: @"C:\runtime\mdCAD.exe",
                RuntimeRoot: @"C:\runtime");
            MdCadLaunchSnapshot snapshot = MdCadLaunchSnapshot.Create(
                jsonlPath,
                startupLiveRefreshEnabled: true,
                viewportOnlyStartupMode: true);

            ProcessStartInfo backendStartInfo = WindowsMdCadEmbedBackend.CreateStartInfo(
                runtime,
                new IntPtr(0xD00D),
                snapshot);
            ProcessStartInfo sharedStartInfo = MdCadWindowsStartInfoBuilder.CreateEmbeddedProcessStartInfo(
                executablePath: runtime.ExecutablePath,
                runtimeRoot: runtime.RuntimeRoot,
                parentHwnd: new IntPtr(0xD00D),
                viewportOnlyStartupMode: snapshot.ViewportOnlyStartupMode,
                launchJsonlPath: snapshot.LaunchJsonlPath,
                startupLiveRefreshEnabled: snapshot.ShouldPassLiveRefreshArgument);

            Assert.Equal(sharedStartInfo.FileName, backendStartInfo.FileName);
            Assert.Equal(sharedStartInfo.WorkingDirectory, backendStartInfo.WorkingDirectory);
            Assert.Equal(sharedStartInfo.ArgumentList, backendStartInfo.ArgumentList);
        }
        finally
        {
            File.Delete(jsonlPath);
        }
    }

    [Fact]
    public void NativeFailureSummary_IsCapturedFromStableEmbeddedSummaryLine()
    {
        const string summary = "Embedded startup failure: bad parent hwnd. See mdcad-embed-crash.log";

        Assert.Equal(summary, InvokeTryCaptureEmbeddedFailureSummary(summary));
    }

    [Fact]
    public void UnexpectedSessionLoss_PrefersCapturedNativeFailureSummary()
    {
        const string capturedSummary =
            "Embedded runtime failure: mdCAD terminated with an unhandled embedded exception (exception=0xC0000005). See mdcad-embed-crash.log";

        string detail = InvokeBuildUnexpectedSessionLossDetail(
            capturedSummary,
            "mdCAD exited after attach.",
            exitCode: 11);

        Assert.Equal(capturedSummary, detail);
    }

    [Fact]
    public void UnexpectedSessionLoss_FallsBackToTruthfulExitStateWhenNoFailureSummaryWasObserved()
    {
        string detail = InvokeBuildUnexpectedSessionLossDetail(
            capturedFailureLine: null,
            fallbackDetail: "mdCAD exited after attach.",
            exitCode: 11);

        Assert.Equal("mdCAD exited after attach (exit code 11).", detail);
    }

    private static string CreateReadableJsonlPath()
    {
        string path = Path.Combine(Path.GetTempPath(), $"mdcad-launch-{Guid.NewGuid():N}.jsonl");
        File.WriteAllText(path, "{}");
        return path;
    }

    private static string? InvokeTryCaptureEmbeddedFailureSummary(string? data)
    {
        MethodInfo method = typeof(WindowsMdCadEmbedBackend).GetMethod(
            "TryCaptureEmbeddedFailureSummary",
            BindingFlags.Static | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing TryCaptureEmbeddedFailureSummary.");

        return (string?)method.Invoke(null, [data]);
    }

    private static string InvokeBuildUnexpectedSessionLossDetail(string? capturedFailureLine, string fallbackDetail, int? exitCode)
    {
        MethodInfo method = typeof(WindowsMdCadEmbedBackend).GetMethod(
            "BuildUnexpectedSessionLossDetail",
            BindingFlags.Static | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing BuildUnexpectedSessionLossDetail.");

        return Assert.IsType<string>(method.Invoke(null, [capturedFailureLine, fallbackDetail, exitCode]));
    }
}
