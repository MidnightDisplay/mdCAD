using System.Diagnostics;
using System.IO;
using System.Security.Cryptography;

using MdCad.Embed.Core;

namespace MdCad.Wpf.Control.Tests;

public sealed class RuntimeRefreshIntegrationTests
{
    private static readonly Lazy<RuntimeIntegrationArtifacts> Artifacts = new(CreateArtifacts);

    [Fact]
    public void RuntimeRefresh_FullHostOutputContainsCanonicalRuntimeBundle()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        RuntimeIntegrationArtifacts artifacts = Artifacts.Value;

        AssertCanonicalRuntimeBundle(
            Path.Combine(artifacts.FullHostBaseDirectory, "mdcad-runtime"),
            artifacts.CommittedRuntimeRoot);
    }

    [Fact]
    public void RuntimeRefresh_MinimalHostOutputContainsCanonicalRuntimeBundle()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        RuntimeIntegrationArtifacts artifacts = Artifacts.Value;

        AssertCanonicalRuntimeBundle(
            Path.Combine(artifacts.MinimalHostBaseDirectory, "mdcad-runtime"),
            artifacts.CommittedRuntimeRoot);
    }

    [Fact]
    public void RuntimeRefresh_WpfHostOutputsStillResolveThroughMdCadRuntimeResolver()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        RuntimeIntegrationArtifacts artifacts = Artifacts.Value;

        AssertRuntimeResolver(artifacts.FullHostBaseDirectory);
        AssertRuntimeResolver(artifacts.MinimalHostBaseDirectory);
    }

    private static RuntimeIntegrationArtifacts CreateArtifacts()
    {
        string repoRoot = FindRepoRoot();
        string fullHostProjectPath = Path.Combine(repoRoot, "samples", "wpf-host", "WpfHost.csproj");
        string minimalHostProjectPath = Path.Combine(repoRoot, "samples", "wpf-host-minimal", "WpfHostMinimal.csproj");

        BuildProject(fullHostProjectPath, repoRoot);
        BuildProject(minimalHostProjectPath, repoRoot);

        return new RuntimeIntegrationArtifacts(
            repoRoot,
            Path.Combine(repoRoot, "samples", "avalonia-mdcad-control", "runtime", "win-x64"),
            GetHostBaseDirectory(repoRoot, "wpf-host"),
            GetHostBaseDirectory(repoRoot, "wpf-host-minimal"));
    }

    private static void AssertCanonicalRuntimeBundle(string hostRuntimeRoot, string committedRuntimeRoot)
    {
        string committedExecutablePath = Path.Combine(committedRuntimeRoot, "mdCAD.exe");
        string committedIniPath = Path.Combine(committedRuntimeRoot, "imgui.embedded.ini");
        string hostExecutablePath = Path.Combine(hostRuntimeRoot, "mdCAD.exe");
        string hostIniPath = Path.Combine(hostRuntimeRoot, "imgui.embedded.ini");

        Assert.Equal(GetFileHash(committedExecutablePath), GetFileHash(hostExecutablePath));
        Assert.Equal(GetFileHash(committedIniPath), GetFileHash(hostIniPath));
    }

    private static void AssertRuntimeResolver(string hostBaseDirectory)
    {
        string expectedRuntimeRoot = Path.Combine(hostBaseDirectory, "mdcad-runtime");
        string expectedExecutablePath = Path.Combine(expectedRuntimeRoot, "mdCAD.exe");
        string expectedIniPath = Path.Combine(expectedRuntimeRoot, "imgui.embedded.ini");

        MdCadRuntimePaths runtime = MdCadRuntimeResolver.ResolveFromBaseDirectory(hostBaseDirectory);

        Assert.Equal(expectedRuntimeRoot, runtime.RuntimeRoot);
        Assert.Equal(expectedExecutablePath, runtime.ExecutablePath);
        Assert.Equal(expectedIniPath, runtime.EmbeddedIniPath);
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
        string? hostBaseDirectory = Directory
            .EnumerateDirectories(hostReleaseRoot, "net10.0-windows*")
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
        string CommittedRuntimeRoot,
        string FullHostBaseDirectory,
        string MinimalHostBaseDirectory);
}
