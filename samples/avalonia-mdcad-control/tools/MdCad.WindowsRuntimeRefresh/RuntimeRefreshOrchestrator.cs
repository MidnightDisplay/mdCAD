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
    private readonly Func<ProcessStartInfo, CancellationToken, Task<int>> _processRunner;

    public RuntimeRefreshOrchestrator()
        : this(RunProcessAsync)
    {
    }

    public RuntimeRefreshOrchestrator(Func<ProcessStartInfo, CancellationToken, Task<int>> processRunner)
    {
        _processRunner = processRunner ?? throw new ArgumentNullException(nameof(processRunner));
    }

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

    public async Task<RuntimeRefreshResult> RunAsync(RuntimeRefreshOptions options, CancellationToken cancellationToken)
    {
        RuntimeRefreshPlan plan = CreatePlan(options);

        if (!File.Exists(plan.CMakeCachePath))
        {
            throw new InvalidOperationException($"Configured CMake cache is missing: {plan.CMakeCachePath}");
        }

        ProcessStartInfo startInfo = plan.CreateBuildStartInfo();
        int exitCode = await _processRunner(startInfo, cancellationToken);
        if (exitCode != 0)
        {
            throw new InvalidOperationException($"cmake --build failed with exit code {exitCode}.");
        }

        if (!File.Exists(plan.SourceExecutablePath))
        {
            throw new InvalidOperationException($"Built mdCAD executable is missing: {plan.SourceExecutablePath}");
        }

        CopyExecutable(plan.SourceExecutablePath, plan.RuntimeExecutablePath);
        if (plan.OutputExecutablePath is not null)
        {
            CopyRuntimeBundle(options.RuntimeDirectory, options.OutputRuntimeDirectory!);
        }

        return new RuntimeRefreshResult(
            SourceExecutablePath: plan.SourceExecutablePath,
            RuntimeExecutablePath: plan.RuntimeExecutablePath,
            OutputExecutablePath: plan.OutputExecutablePath);
    }

    private static void CopyExecutable(string sourceExecutablePath, string targetExecutablePath)
    {
        string? targetDirectory = Path.GetDirectoryName(targetExecutablePath);
        if (string.IsNullOrEmpty(targetDirectory))
        {
            throw new InvalidOperationException($"Target executable path has no directory: {targetExecutablePath}");
        }

        Directory.CreateDirectory(targetDirectory);
        File.Copy(sourceExecutablePath, targetExecutablePath, overwrite: true);
    }

    private static void CopyRuntimeBundle(string sourceRuntimeDirectory, string targetRuntimeDirectory)
    {
        Directory.CreateDirectory(targetRuntimeDirectory);

        foreach (string sourceFilePath in Directory.EnumerateFiles(sourceRuntimeDirectory, "*", SearchOption.AllDirectories))
        {
            string relativePath = Path.GetRelativePath(sourceRuntimeDirectory, sourceFilePath);
            string targetFilePath = Path.Combine(targetRuntimeDirectory, relativePath);
            string? targetDirectory = Path.GetDirectoryName(targetFilePath);
            if (!string.IsNullOrEmpty(targetDirectory))
            {
                Directory.CreateDirectory(targetDirectory);
            }

            File.Copy(sourceFilePath, targetFilePath, overwrite: true);
        }
    }

    private static async Task<int> RunProcessAsync(ProcessStartInfo startInfo, CancellationToken cancellationToken)
    {
        using Process process = new()
        {
            StartInfo = startInfo
        };

        if (!process.Start())
        {
            throw new InvalidOperationException($"Failed to start process: {startInfo.FileName}");
        }

        await process.WaitForExitAsync(cancellationToken);
        return process.ExitCode;
    }
}
