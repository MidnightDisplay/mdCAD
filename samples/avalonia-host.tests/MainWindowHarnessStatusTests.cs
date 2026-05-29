using System.IO;
using System.Linq;
using System.Reflection;
using System.Xml.Linq;

using AvaloniaHost;

namespace AvaloniaHost.Tests;

public sealed class MainWindowHarnessStatusTests
{
    [Fact]
    public void UnexpectedSessionLossDetail_IsMirroredVerbatimInHarnessStatus()
    {
        const string detail =
            "Embedded runtime failure: mdCAD terminated with an unhandled embedded exception (exception=0xC0000005). See mdcad-embed-crash.log";

        string status = InvokeComposeHarnessStatus(detail);

        Assert.Contains("mode=diagnostic", status);
        Assert.Contains($"observed detail: {detail}", status);
    }

    private static string InvokeComposeHarnessStatus(string detail)
    {
        MethodInfo method = typeof(MainWindow).GetMethod(
            "ComposeHarnessStatus",
            BindingFlags.Static | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing ComposeHarnessStatus.");

        return Assert.IsType<string>(method.Invoke(null, ["diagnostic", "false", "off", "jsonl=none", null, detail]));
    }

    [Fact]
    public void HostProjects_EnablePinnedRuntimeRefreshOnControlReference()
    {
        string repoRoot = FindRepoRoot();

        AssertRuntimeRefreshEnabled(
            Path.Combine(repoRoot, "samples", "avalonia-host", "AvaloniaHost.csproj"),
            @"..\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj");
        AssertRuntimeRefreshEnabled(
            Path.Combine(repoRoot, "samples", "avalonia-host-minimal", "AvaloniaHostMinimal.csproj"),
            @"..\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj");
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
