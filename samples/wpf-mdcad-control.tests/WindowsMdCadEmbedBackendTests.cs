using System.Diagnostics;
using System.IO;
using System.Xml.Linq;

using MdCad.Embed.Core;
using MdCad.Embed.Core.Windows;
using MdCad.Wpf.Control.Host.Windows;

namespace MdCad.Wpf.Control.Tests;

public sealed class WindowsMdCadEmbedBackendTests : IClassFixture<WpfStaThreadFixture>
{
    private readonly WpfStaThreadFixture _sta;

    public WindowsMdCadEmbedBackendTests(WpfStaThreadFixture sta)
    {
        _sta = sta;
    }

    [Fact]
    public async Task Backend_UsesRealHwndHostSurface()
    {
        await _sta.RunAsync(async () =>
        {
            WindowsMdCadEmbedBackend backend = CreateBackend();

            await Task.Yield();

            Assert.IsType<MdCadHwndHost>(backend.Surface);
        });
    }

    [Fact]
    public void CreateStartInfo_DelegatesToSharedWindowsStartInfoBuilder()
    {
        string jsonlPath = CreateReadableJsonlPath();
        try
        {
            MdCadRuntimePaths runtime = new(
                ExecutablePath: @"C:\runtime\mdCAD.exe",
                RuntimeRoot: @"C:\runtime",
                EmbeddedIniPath: @"C:\runtime\imgui.embedded.ini");
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
    public void ProjectFile_UsesCanonicalRuntimeBundleAndRefreshHelper()
    {
        XDocument project = XDocument.Load(Path.Combine(
            AppContext.BaseDirectory,
            "..",
            "..",
            "..",
            "..",
            "wpf-mdcad-control",
            "MdCad.Wpf.Control.csproj"));

        string normalizedXml = project.ToString();

        Assert.Contains(@"..\avalonia-mdcad-control\runtime\win-x64\**\*", normalizedXml, StringComparison.Ordinal);
        Assert.Contains("mdcad-runtime", normalizedXml, StringComparison.Ordinal);
        Assert.Contains(@"..\avalonia-mdcad-control\tools\MdCad.WindowsRuntimeRefresh\MdCad.WindowsRuntimeRefresh.csproj", normalizedXml, StringComparison.Ordinal);
    }

    [Fact]
    public void TryGetRelevantFailureLine_CapturesEmbeddedStartupSummary()
    {
        const string startupSummary = "Embedded startup failure: Embedded startup failed: bad parent hwnd. See mdcad-embed-crash.log";

        bool captured = WindowsMdCadEmbedBackend.TryGetRelevantFailureLine(startupSummary, out string? relevantLine);

        Assert.True(captured);
        Assert.Equal(startupSummary, relevantLine);
    }

    [Fact]
    public void ComposeUnexpectedSessionLossDetail_PrefersCapturedRuntimeSummaryAfterAttach()
    {
        const string runtimeSummary = "Embedded runtime failure: access violation (exception=0xC0000005). See mdcad-embed-crash.log";

        string detail = WindowsMdCadEmbedBackend.ComposeUnexpectedSessionLossDetail(
            runtimeSummary,
            "mdCAD exited after child attach (exit code 23).");

        Assert.Equal(runtimeSummary, detail);
    }

    [Fact]
    public void ComposeUnexpectedSessionLossDetail_UsesTruthfulFallbackWhenNoFailureSummaryWasObserved()
    {
        const string fallback = "mdCAD exited after child attach (exit code 23).";

        string detail = WindowsMdCadEmbedBackend.ComposeUnexpectedSessionLossDetail(null, fallback);

        Assert.Equal(fallback, detail);
    }

    private static WindowsMdCadEmbedBackend CreateBackend()
    {
        return new WindowsMdCadEmbedBackend(
            initialSnapshot: MdCadLaunchSnapshot.Create(null, startupLiveRefreshEnabled: false, viewportOnlyStartupMode: false),
            setLaunchStatus: _ => { },
            setAttachStatus: _ => { },
            updatePreLaunchStatus: _ => { },
            updatePostLaunchStatus: _ => { },
            setFailureStatus: _ => { },
            setLaunchWarning: _ => { },
            updateControlState: () => { },
            captureLaunchSnapshot: () => MdCadLaunchSnapshot.Create(null, startupLiveRefreshEnabled: false, viewportOnlyStartupMode: false));
    }

    private static string CreateReadableJsonlPath()
    {
        string path = Path.Combine(Path.GetTempPath(), $"mdcad-wpf-backend-{Guid.NewGuid():N}.jsonl");
        File.WriteAllText(path, "{}");
        return path;
    }
}
