using System.IO;
using System.Linq;
using System.Reflection;
using System.Threading;
using System.Xml.Linq;

using MdCad.Embed.Core;

namespace WpfHost.Tests;

public sealed class MainWindowHarnessStatusTests
{
    [Fact]
    public async Task HarnessStatus_MirrorsObservedUnexpectedSessionLossDetailVerbatim()
    {
        const string detail = "Embedded runtime failure: access violation (exception=0xC0000005). See mdcad-embed-crash.log";

        await RunStaAsync(() =>
        {
            MainWindow window = new(new HostLaunchOptions(
                MdCadPresentationMode.Diagnostic,
                HostJsonlScenario.Empty,
                autoStart: false,
                requestLiveRefresh: false,
                viewportOnlyStartupMode: false,
                parseError: null));

            MethodInfo handler = typeof(MainWindow).GetMethod(
                "OnEmbeddedControlUnexpectedSessionLoss",
                BindingFlags.Instance | BindingFlags.NonPublic)
                ?? throw new InvalidOperationException("Missing unexpected-session-loss relay handler.");
            MethodInfo buildHarnessStatus = typeof(MainWindow).GetMethod(
                "BuildHarnessStatus",
                BindingFlags.Instance | BindingFlags.NonPublic)
                ?? throw new InvalidOperationException("Missing BuildHarnessStatus method.");

            handler.Invoke(window, [detail]);
            string status = Assert.IsType<string>(buildHarnessStatus.Invoke(window, null));

            Assert.Contains(detail, status, StringComparison.Ordinal);
            Assert.Contains("viewer state remains control-managed", status, StringComparison.Ordinal);
        });
    }

    private static Task RunStaAsync(Action action)
    {
        TaskCompletionSource completion = new(TaskCreationOptions.RunContinuationsAsynchronously);
        Thread thread = new(() =>
        {
            try
            {
                action();
                completion.SetResult();
            }
            catch (Exception ex)
            {
                completion.SetException(ex);
            }
        });

        thread.SetApartmentState(ApartmentState.STA);
        thread.IsBackground = true;
        thread.Start();
        return completion.Task;
    }

    [Fact]
    public void HostProjects_EnablePinnedRuntimeRefreshOnControlReference()
    {
        string repoRoot = FindRepoRoot();

        AssertRuntimeRefreshEnabled(
            Path.Combine(repoRoot, "samples", "wpf-host", "WpfHost.csproj"),
            @"..\wpf-mdcad-control\MdCad.Wpf.Control.csproj");
        AssertRuntimeRefreshEnabled(
            Path.Combine(repoRoot, "samples", "wpf-host-minimal", "WpfHostMinimal.csproj"),
            @"..\wpf-mdcad-control\MdCad.Wpf.Control.csproj");
    }

    private static void AssertRuntimeRefreshEnabled(string projectPath, string expectedReference)
    {
        XDocument project = XDocument.Load(projectPath);
        XElement reference = project
            .Descendants("ProjectReference")
            .FirstOrDefault(element => string.Equals((string?)element.Attribute("Include"), expectedReference, StringComparison.OrdinalIgnoreCase))
            ?? throw new InvalidOperationException($"Missing control project reference in {projectPath}");

        string additionalProperties = reference.Element("AdditionalProperties")?.Value
            ?? throw new InvalidOperationException($"Missing AdditionalProperties for control reference in {projectPath}");

        Assert.Contains("MdCadRefreshWindowsRuntime=true", additionalProperties, StringComparison.OrdinalIgnoreCase);

        XElement target = project
            .Descendants("Target")
            .FirstOrDefault(element => string.Equals((string?)element.Attribute("Name"), "RefreshPinnedWindowsRuntime", StringComparison.OrdinalIgnoreCase))
            ?? throw new InvalidOperationException($"Missing RefreshPinnedWindowsRuntime target in {projectPath}");

        XElement exec = target
            .Descendants("Exec")
            .FirstOrDefault()
            ?? throw new InvalidOperationException($"Missing runtime refresh Exec task in {projectPath}");

        string command = (string?)exec.Attribute("Command")
            ?? throw new InvalidOperationException($"Missing runtime refresh command in {projectPath}");

        Assert.Contains("--output-runtime-dir", command, StringComparison.OrdinalIgnoreCase);
    }

    private static string FindRepoRoot()
    {
        DirectoryInfo? current = new(AppContext.BaseDirectory);
        while (current is not null)
        {
            string planningPath = Path.Combine(current.FullName, ".planning");
            string gitPath = Path.Combine(current.FullName, ".git");
            if (Directory.Exists(planningPath) && (Directory.Exists(gitPath) || File.Exists(gitPath)))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new InvalidOperationException("Could not locate the mdCAD repository root from the test output directory.");
    }
}
