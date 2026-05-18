using System.IO;
using System.Security.Cryptography;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control.Tests;

public sealed class RuntimeRefreshIntegrationTests
{
    [Fact]
    public void RuntimeRefresh_HostOutputExecutableMatchesBuildAndCommittedRuntime()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        string repoRoot = FindRepoRoot();
        string sourceExecutablePath = Path.Combine(repoRoot, "build-vulkan", "bin", "Release", "mdCAD.exe");
        string committedExecutablePath = Path.Combine(repoRoot, "samples", "avalonia-mdcad-control", "runtime", "win-x64", "mdCAD.exe");
        string hostRuntimeRoot = GetHostRuntimeRoot(repoRoot);
        string hostExecutablePath = Path.Combine(hostRuntimeRoot, "mdCAD.exe");

        string sourceHash = GetFileHash(sourceExecutablePath);
        string committedHash = GetFileHash(committedExecutablePath);
        string hostHash = GetFileHash(hostExecutablePath);

        Assert.Equal(sourceHash, committedHash);
        Assert.Equal(sourceHash, hostHash);
    }

    [Fact]
    public void RuntimeRefresh_HostOutputStillResolvesThroughMdCadRuntimeResolver()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        string repoRoot = FindRepoRoot();
        string hostBaseDirectory = GetHostBaseDirectory(repoRoot);
        string expectedRuntimeRoot = Path.Combine(hostBaseDirectory, "mdcad-runtime");
        string expectedExecutablePath = Path.Combine(expectedRuntimeRoot, "mdCAD.exe");

        MdCadRuntimePaths runtime = MdCadRuntimeResolver.ResolveFromBaseDirectory(hostBaseDirectory);

        Assert.Equal(expectedRuntimeRoot, runtime.RuntimeRoot);
        Assert.Equal(expectedExecutablePath, runtime.ExecutablePath);
    }

    [Fact]
    public void RuntimeRefresh_HostOutputRetainsEmbeddedIniBundleEntry()
    {
        Assert.True(OperatingSystem.IsWindows(), "Runtime refresh integration tests require Windows.");

        string repoRoot = FindRepoRoot();
        string committedIniPath = Path.Combine(repoRoot, "samples", "avalonia-mdcad-control", "runtime", "win-x64", "imgui.embedded.ini");
        string hostIniPath = Path.Combine(GetHostRuntimeRoot(repoRoot), "imgui.embedded.ini");

        Assert.True(File.Exists(committedIniPath), $"Committed embedded ini is missing: {committedIniPath}");
        Assert.True(File.Exists(hostIniPath), $"Host embedded ini is missing: {hostIniPath}");
    }

    private static string FindRepoRoot()
    {
        DirectoryInfo? current = new(AppContext.BaseDirectory);
        while (current is not null)
        {
            string planningPath = Path.Combine(current.FullName, ".planning");
            string gitPath = Path.Combine(current.FullName, ".git");
            if (Directory.Exists(planningPath) && Directory.Exists(gitPath))
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        throw new InvalidOperationException("Could not locate the mdCAD repository root from the test output directory.");
    }

    private static string GetHostBaseDirectory(string repoRoot)
    {
        string hostReleaseRoot = Path.Combine(repoRoot, "samples", "avalonia-host", "bin", "Release");
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

    private static string GetHostRuntimeRoot(string repoRoot)
    {
        return Path.Combine(GetHostBaseDirectory(repoRoot), "mdcad-runtime");
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
}
