using System.Diagnostics;
using System.IO;

using MdCad.Avalonia.Control.Tools.RuntimeRefresh;

namespace MdCad.Avalonia.Control.Tests;

public sealed class RuntimeRefreshHelperTests
{
    [Fact]
    public void Parse_MissingBuildDirectory_Throws()
    {
        ArgumentException ex = Assert.Throws<ArgumentException>(
            () => RuntimeRefreshOptions.Parse(
                new[]
                {
                    "--repo-root", @"C:\repo",
                    "--runtime-dir", @"C:\repo\runtime\win-x64",
                    "--configuration", "Release"
                }));

        Assert.Contains("--build-dir", ex.Message);
    }

    [Fact]
    public void Parse_NormalizesConfiguredPathsToAbsolute()
    {
        string root = CreateTempDirectory();
        try
        {
            string repoRoot = Path.Combine(root, "repo");
            string buildDir = Path.Combine(repoRoot, "build-vulkan");
            string runtimeDir = Path.Combine(repoRoot, "samples", "avalonia-mdcad-control", "runtime", "win-x64");
            string outputDir = Path.Combine(root, "artifacts", "mdcad-runtime");

            Directory.CreateDirectory(buildDir);
            Directory.CreateDirectory(runtimeDir);
            Directory.CreateDirectory(outputDir);

            RuntimeRefreshOptions options = RuntimeRefreshOptions.Parse(
                new[]
                {
                    "--repo-root", Path.GetRelativePath(Environment.CurrentDirectory, repoRoot),
                    "--build-dir", Path.GetRelativePath(Environment.CurrentDirectory, buildDir),
                    "--runtime-dir", Path.GetRelativePath(Environment.CurrentDirectory, runtimeDir),
                    "--output-runtime-dir", Path.GetRelativePath(Environment.CurrentDirectory, outputDir),
                    "--configuration", "Release"
                });

            Assert.Equal(Path.GetFullPath(repoRoot), options.RepoRoot);
            Assert.Equal(Path.GetFullPath(buildDir), options.BuildDirectory);
            Assert.Equal(Path.GetFullPath(runtimeDir), options.RuntimeDirectory);
            Assert.Equal(Path.GetFullPath(outputDir), options.OutputRuntimeDirectory);
            Assert.Equal("Release", options.Configuration);
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    [Fact]
    public void CreatePlan_DerivesBuildOutputAndCommittedRuntimeExecutable()
    {
        RuntimeRefreshOptions options = RuntimeRefreshOptions.Parse(
            new[]
            {
                "--repo-root", @"C:\repo",
                "--build-dir", @"C:\repo\build-vulkan",
                "--runtime-dir", @"C:\repo\samples\avalonia-mdcad-control\runtime\win-x64",
                "--configuration", "Release"
            });

        RuntimeRefreshPlan plan = new RuntimeRefreshOrchestrator().CreatePlan(options);

        Assert.Equal(Path.Combine(@"C:\repo\build-vulkan", "CMakeCache.txt"), plan.CMakeCachePath);
        Assert.Equal(Path.Combine(@"C:\repo\build-vulkan", "bin", "Release", "mdCAD.exe"), plan.SourceExecutablePath);
        Assert.Equal(Path.Combine(@"C:\repo\samples\avalonia-mdcad-control\runtime\win-x64", "mdCAD.exe"), plan.RuntimeExecutablePath);
        Assert.Null(plan.OutputExecutablePath);
        Assert.Single(plan.CopyTargets);
        Assert.All(plan.CopyTargets, target => Assert.EndsWith("mdCAD.exe", target, StringComparison.Ordinal));
    }

    [Fact]
    public void CreatePlan_OutputRuntimeDirectoryAddsCurrentOutputWithoutReplacingCommittedRuntime()
    {
        RuntimeRefreshOptions options = RuntimeRefreshOptions.Parse(
            new[]
            {
                "--repo-root", @"C:\repo",
                "--build-dir", @"C:\repo\build-vulkan",
                "--runtime-dir", @"C:\repo\samples\avalonia-mdcad-control\runtime\win-x64",
                "--output-runtime-dir", @"C:\repo\bin\Release\net10.0\mdcad-runtime",
                "--configuration", "Release"
            });

        RuntimeRefreshPlan plan = new RuntimeRefreshOrchestrator().CreatePlan(options);

        Assert.Equal(2, plan.CopyTargets.Count);
        Assert.Equal(plan.RuntimeExecutablePath, plan.CopyTargets[0]);
        Assert.Equal(plan.OutputExecutablePath, plan.CopyTargets[1]);
        Assert.DoesNotContain(plan.CopyTargets, target => target.EndsWith("imgui.embedded.ini", StringComparison.OrdinalIgnoreCase));
    }

    [Fact]
    public async Task RunAsync_MissingConfiguredBuildCache_ThrowsClearError()
    {
        string root = CreateTempDirectory();
        try
        {
            string repoRoot = Path.Combine(root, "repo");
            string buildDir = Path.Combine(repoRoot, "build-vulkan");
            string runtimeDir = Path.Combine(repoRoot, "samples", "avalonia-mdcad-control", "runtime", "win-x64");

            Directory.CreateDirectory(buildDir);
            Directory.CreateDirectory(runtimeDir);

            RuntimeRefreshOptions options = RuntimeRefreshOptions.Parse(
                new[]
                {
                    "--repo-root", repoRoot,
                    "--build-dir", buildDir,
                    "--runtime-dir", runtimeDir,
                    "--configuration", "Release"
                });

            RuntimeRefreshOrchestrator orchestrator = new((_, _) => Task.FromResult(0));

            InvalidOperationException ex = await Assert.ThrowsAsync<InvalidOperationException>(
                () => orchestrator.RunAsync(options, CancellationToken.None));

            Assert.Contains("Configured CMake cache is missing", ex.Message);
            Assert.Contains(Path.Combine(buildDir, "CMakeCache.txt"), ex.Message);
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    [Fact]
    public async Task RunAsync_UsesDirectCMakeBuildAndCopiesExecutableTargets()
    {
        string root = CreateTempDirectory();
        try
        {
            string repoRoot = Path.Combine(root, "repo");
            string buildDir = Path.Combine(repoRoot, "build-vulkan");
            string runtimeDir = Path.Combine(repoRoot, "samples", "avalonia-mdcad-control", "runtime", "win-x64");
            string outputDir = Path.Combine(repoRoot, "artifacts", "mdcad-runtime");
            string builtExecutablePath = Path.Combine(buildDir, "bin", "Release", "mdCAD.exe");
            string runtimeIniPath = Path.Combine(runtimeDir, "imgui.embedded.ini");
            string outputIniPath = Path.Combine(outputDir, "imgui.embedded.ini");

            Directory.CreateDirectory(Path.Combine(buildDir, "bin", "Release"));
            Directory.CreateDirectory(runtimeDir);
            Directory.CreateDirectory(outputDir);
            File.WriteAllText(Path.Combine(buildDir, "CMakeCache.txt"), "configured");
            File.WriteAllText(builtExecutablePath, "fresh-mdcad");
            File.WriteAllText(runtimeIniPath, "runtime-seed");
            File.WriteAllText(outputIniPath, "output-state");

            RuntimeRefreshOptions options = RuntimeRefreshOptions.Parse(
                new[]
                {
                    "--repo-root", repoRoot,
                    "--build-dir", buildDir,
                    "--runtime-dir", runtimeDir,
                    "--output-runtime-dir", outputDir,
                    "--configuration", "Release"
                });

            ProcessStartInfo? capturedStartInfo = null;
            RuntimeRefreshOrchestrator orchestrator = new((startInfo, _) =>
            {
                capturedStartInfo = startInfo;
                return Task.FromResult(0);
            });

            RuntimeRefreshResult result = await orchestrator.RunAsync(options, CancellationToken.None);

            Assert.NotNull(capturedStartInfo);
            Assert.Equal("cmake", capturedStartInfo!.FileName);
            Assert.False(capturedStartInfo.UseShellExecute);
            Assert.Equal(repoRoot, capturedStartInfo.WorkingDirectory);
            Assert.Equal(new[] { "--build", buildDir, "--config", "Release", "--target", "mdCAD" }, capturedStartInfo.ArgumentList);
            Assert.DoesNotContain("powershell", string.Join(" ", capturedStartInfo.ArgumentList), StringComparison.OrdinalIgnoreCase);
            Assert.DoesNotContain("cmd", string.Join(" ", capturedStartInfo.ArgumentList), StringComparison.OrdinalIgnoreCase);

            string runtimeExecutablePath = Path.Combine(runtimeDir, "mdCAD.exe");
            string outputExecutablePath = Path.Combine(outputDir, "mdCAD.exe");

            Assert.Equal(builtExecutablePath, result.SourceExecutablePath);
            Assert.Equal(runtimeExecutablePath, result.RuntimeExecutablePath);
            Assert.Equal(outputExecutablePath, result.OutputExecutablePath);
            Assert.Equal("fresh-mdcad", File.ReadAllText(runtimeExecutablePath));
            Assert.Equal("fresh-mdcad", File.ReadAllText(outputExecutablePath));
            Assert.Equal("runtime-seed", File.ReadAllText(runtimeIniPath));
            Assert.Equal("runtime-seed", File.ReadAllText(outputIniPath));
        }
        finally
        {
            Directory.Delete(root, recursive: true);
        }
    }

    private static string CreateTempDirectory()
    {
        string path = Path.Combine(Path.GetTempPath(), $"mdcad-runtime-refresh-tests-{Guid.NewGuid():N}");
        Directory.CreateDirectory(path);
        return path;
    }
}
