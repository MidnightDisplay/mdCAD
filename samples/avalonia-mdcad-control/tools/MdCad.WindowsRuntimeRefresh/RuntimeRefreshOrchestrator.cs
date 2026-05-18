using System.Collections.ObjectModel;
using System.Diagnostics;
using System.IO;

namespace MdCad.Avalonia.Control.Tools.RuntimeRefresh;

public sealed record RuntimeRefreshPlan(
    string RepoRoot,
    string BuildDirectory,
    string Configuration,
    string CMakeCachePath,
    string SourceExecutablePath,
    string RuntimeExecutablePath,
    string? OutputExecutablePath)
{
    public IReadOnlyList<string> CopyTargets { get; } = CreateCopyTargets(RuntimeExecutablePath, OutputExecutablePath);

    public ProcessStartInfo CreateBuildStartInfo()
    {
        ProcessStartInfo startInfo = new("cmake")
        {
            UseShellExecute = false,
            WorkingDirectory = RepoRoot
        };

        startInfo.ArgumentList.Add("--build");
        startInfo.ArgumentList.Add(BuildDirectory);
        startInfo.ArgumentList.Add("--config");
        startInfo.ArgumentList.Add(Configuration);
        startInfo.ArgumentList.Add("--target");
        startInfo.ArgumentList.Add("mdCAD");
        return startInfo;
    }

    private static IReadOnlyList<string> CreateCopyTargets(string runtimeExecutablePath, string? outputExecutablePath)
    {
        List<string> targets = new()
        {
            runtimeExecutablePath
        };

        if (!string.IsNullOrEmpty(outputExecutablePath))
        {
            targets.Add(outputExecutablePath);
        }

        return new ReadOnlyCollection<string>(targets);
    }
}

public sealed record RuntimeRefreshResult(
    string SourceExecutablePath,
    string RuntimeExecutablePath,
    string? OutputExecutablePath);

public sealed class RuntimeRefreshOrchestrator
{
    private const string ExecutableName = "mdCAD.exe";

    public RuntimeRefreshPlan CreatePlan(RuntimeRefreshOptions options)
    {
        ArgumentNullException.ThrowIfNull(options);

        string sourceExecutablePath = Path.Combine(
            options.BuildDirectory,
            "bin",
            options.Configuration,
            ExecutableName);

        string runtimeExecutablePath = Path.Combine(options.RuntimeDirectory, ExecutableName);
        string? outputExecutablePath = options.OutputRuntimeDirectory is null
            ? null
            : Path.Combine(options.OutputRuntimeDirectory, ExecutableName);

        return new RuntimeRefreshPlan(
            RepoRoot: options.RepoRoot,
            BuildDirectory: options.BuildDirectory,
            Configuration: options.Configuration,
            CMakeCachePath: Path.Combine(options.BuildDirectory, "CMakeCache.txt"),
            SourceExecutablePath: sourceExecutablePath,
            RuntimeExecutablePath: runtimeExecutablePath,
            OutputExecutablePath: outputExecutablePath);
    }

    public Task<RuntimeRefreshResult> RunAsync(RuntimeRefreshOptions options, CancellationToken cancellationToken)
    {
        _ = CreatePlan(options);
        _ = cancellationToken;
        throw new NotSupportedException("Runtime refresh execution is added in the next task.");
    }
}
