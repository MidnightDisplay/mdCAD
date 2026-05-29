using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control.Tests;

public sealed class RuntimeRefreshIntegrationTests
{
    private static readonly Lazy<RuntimeIntegrationArtifacts> Artifacts = new(CreateArtifacts);

    [Fact]
    public void RuntimeRefresh_FullHostOutputExecutableMatchesBuildAndCommittedRuntime()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        RuntimeIntegrationArtifacts artifacts = Artifacts.Value;

        AssertCanonicalRuntimeBundle(
            Path.Combine(artifacts.FullHostBaseDirectory, "mdcad-runtime"),
            artifacts.CommittedRuntimeRoot,
            artifacts.SourceExecutablePath);
    }

    [Fact]
    public void RuntimeRefresh_MinimalHostOutputExecutableMatchesBuildAndCommittedRuntime()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        RuntimeIntegrationArtifacts artifacts = Artifacts.Value;

        AssertCanonicalRuntimeBundle(
            Path.Combine(artifacts.MinimalHostBaseDirectory, "mdcad-runtime"),
            artifacts.CommittedRuntimeRoot,
            artifacts.SourceExecutablePath);
    }

    [Fact]
    public void RuntimeRefresh_HostOutputsStillResolveThroughMdCadRuntimeResolver()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        RuntimeIntegrationArtifacts artifacts = Artifacts.Value;

        AssertRuntimeResolver(artifacts.FullHostBaseDirectory);
        AssertRuntimeResolver(artifacts.MinimalHostBaseDirectory);
    }

    [Fact]
    public void RuntimeRefresh_HostOutputsRetainEmbeddedIniBundleEntry()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        RuntimeIntegrationArtifacts artifacts = Artifacts.Value;
        string committedIniPath = Path.Combine(artifacts.CommittedRuntimeRoot, "imgui.embedded.ini");

        Assert.True(File.Exists(committedIniPath), $"Committed embedded ini is missing: {committedIniPath}");
        Assert.True(File.Exists(Path.Combine(artifacts.FullHostBaseDirectory, "mdcad-runtime", "imgui.embedded.ini")));
        Assert.True(File.Exists(Path.Combine(artifacts.MinimalHostBaseDirectory, "mdcad-runtime", "imgui.embedded.ini")));
    }

    private static RuntimeIntegrationArtifacts CreateArtifacts()
    {
        string repoRoot = FindRepoRoot();
        string fullHostProjectPath = Path.Combine(repoRoot, "samples", "avalonia-host", "AvaloniaHost.csproj");
        string minimalHostProjectPath = Path.Combine(repoRoot, "samples", "avalonia-host-minimal", "AvaloniaHostMinimal.csproj");
        string sourceExecutablePath = Path.Combine(repoRoot, "build-vulkan", "bin", "Release", "mdCAD.exe");

        BuildProject(fullHostProjectPath, repoRoot);
        BuildProject(minimalHostProjectPath, repoRoot);

        return new RuntimeIntegrationArtifacts(
            repoRoot,
            sourceExecutablePath,
            Path.Combine(repoRoot, "samples", "avalonia-mdcad-control", "runtime", "win-x64"),
            GetHostBaseDirectory(repoRoot, "avalonia-host"),
            GetHostBaseDirectory(repoRoot, "avalonia-host-minimal"));
    }

    private static void AssertCanonicalRuntimeBundle(string hostRuntimeRoot, string committedRuntimeRoot, string sourceExecutablePath)
    {
        string committedExecutablePath = Path.Combine(committedRuntimeRoot, "mdCAD.exe");
        string hostExecutablePath = Path.Combine(hostRuntimeRoot, "mdCAD.exe");

        Assert.Equal(GetFileHash(sourceExecutablePath), GetFileHash(committedExecutablePath));
        Assert.Equal(GetFileHash(sourceExecutablePath), GetFileHash(hostExecutablePath));
    }

    private static void AssertRuntimeResolver(string hostBaseDirectory)
    {
        string expectedRuntimeRoot = Path.Combine(hostBaseDirectory, "mdcad-runtime");
        string expectedExecutablePath = Path.Combine(expectedRuntimeRoot, "mdCAD.exe");

        MdCadRuntimePaths runtime = MdCadRuntimeResolver.ResolveFromBaseDirectory(hostBaseDirectory);

        Assert.Equal(expectedRuntimeRoot, runtime.RuntimeRoot);
        Assert.Equal(expectedExecutablePath, runtime.ExecutablePath);
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

    private static string GetHostBaseDirectory(string repoRoot, string hostDirectoryName)
    {
        string hostReleaseRoot = Path.Combine(repoRoot, "samples", hostDirectoryName, "bin", "Release");
        string directoryPattern = string.Equals(hostDirectoryName, "avalonia-host-minimal", StringComparison.OrdinalIgnoreCase)
            ? "net10.0"
            : "net10.0-windows*";
        string? hostBaseDirectory = Directory
            .EnumerateDirectories(hostReleaseRoot, directoryPattern)
            .OrderBy(path => path, StringComparer.OrdinalIgnoreCase)
            .LastOrDefault();

        if (hostBaseDirectory is null)
        {
            throw new InvalidOperationException($"Host build output is missing under: {hostReleaseRoot}");
        }

        return hostBaseDirectory;
    }

    private static void BuildProject(string projectPath, string workingDirectory)
    {
        ProcessStartInfo startInfo = new("dotnet")
        {
            WorkingDirectory = workingDirectory,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            UseShellExecute = false,
        };

        startInfo.ArgumentList.Add("build");
        startInfo.ArgumentList.Add(projectPath);
        startInfo.ArgumentList.Add("-c");
        startInfo.ArgumentList.Add("Release");
        startInfo.ArgumentList.Add("--nologo");

        using Process process = Process.Start(startInfo)
            ?? throw new InvalidOperationException($"Failed to start build for {projectPath}");

        string standardOutput = process.StandardOutput.ReadToEnd();
        string standardError = process.StandardError.ReadToEnd();
        process.WaitForExit();

        if (process.ExitCode != 0)
        {
            throw new InvalidOperationException(
                $"Build failed for {projectPath}.{Environment.NewLine}{standardOutput}{Environment.NewLine}{standardError}");
        }
    }

    private static string GetFileHash(string path)
    {
        if (!File.Exists(path))
        {
            throw new InvalidOperationException($"Expected runtime artifact is missing: {path}");
        }

        using FileStream stream = File.OpenRead(path);
        byte[] hash = SHA256.HashData(stream);
        return Convert.ToHexString(hash);
    }

    private sealed record RuntimeIntegrationArtifacts(
        string RepoRoot,
        string SourceExecutablePath,
        string CommittedRuntimeRoot,
        string FullHostBaseDirectory,
        string MinimalHostBaseDirectory);
}
