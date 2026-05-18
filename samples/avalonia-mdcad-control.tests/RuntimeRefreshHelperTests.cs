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

    private static string CreateTempDirectory()
    {
        string path = Path.Combine(Path.GetTempPath(), $"mdcad-runtime-refresh-tests-{Guid.NewGuid():N}");
        Directory.CreateDirectory(path);
        return path;
    }
}
