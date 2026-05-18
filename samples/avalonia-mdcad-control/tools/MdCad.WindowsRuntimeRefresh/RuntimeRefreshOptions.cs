using System.Collections.Generic;
using System.IO;

namespace MdCad.Avalonia.Control.Tools.RuntimeRefresh;

public sealed record RuntimeRefreshOptions(
    string RepoRoot,
    string BuildDirectory,
    string RuntimeDirectory,
    string? OutputRuntimeDirectory,
    string Configuration)
{
    public static RuntimeRefreshOptions Parse(IReadOnlyList<string> args)
    {
        ArgumentNullException.ThrowIfNull(args);

        Dictionary<string, string> values = new(StringComparer.Ordinal);
        for (int i = 0; i < args.Count; i++)
        {
            string argument = args[i];
            if (!argument.StartsWith("--", StringComparison.Ordinal))
            {
                throw new ArgumentException($"Unexpected argument '{argument}'. Expected a --name value pair.", nameof(args));
            }

            if (i == args.Count - 1)
            {
                throw new ArgumentException($"Argument '{argument}' requires a value.", nameof(args));
            }

            string value = args[++i];
            if (string.IsNullOrWhiteSpace(value))
            {
                throw new ArgumentException($"Argument '{argument}' requires a non-empty value.", nameof(args));
            }

            if (!IsSupportedArgument(argument))
            {
                throw new ArgumentException($"Unknown argument '{argument}'.", nameof(args));
            }

            if (!values.TryAdd(argument, value))
            {
                throw new ArgumentException($"Argument '{argument}' was provided more than once.", nameof(args));
            }
        }

        return new RuntimeRefreshOptions(
            RepoRoot: NormalizeRequiredPath(values, "--repo-root"),
            BuildDirectory: NormalizeRequiredPath(values, "--build-dir"),
            RuntimeDirectory: NormalizeRequiredPath(values, "--runtime-dir"),
            OutputRuntimeDirectory: NormalizeOptionalPath(values, "--output-runtime-dir"),
            Configuration: GetRequiredValue(values, "--configuration"));
    }

    private static bool IsSupportedArgument(string argument)
    {
        return argument is "--repo-root"
            or "--build-dir"
            or "--runtime-dir"
            or "--output-runtime-dir"
            or "--configuration";
    }

    private static string NormalizeRequiredPath(IReadOnlyDictionary<string, string> values, string key)
    {
        return Path.GetFullPath(GetRequiredValue(values, key));
    }

    private static string? NormalizeOptionalPath(IReadOnlyDictionary<string, string> values, string key)
    {
        return values.TryGetValue(key, out string? value) ? Path.GetFullPath(value) : null;
    }

    private static string GetRequiredValue(IReadOnlyDictionary<string, string> values, string key)
    {
        if (!values.TryGetValue(key, out string? value))
        {
            throw new ArgumentException($"Missing required argument '{key}'.", nameof(values));
        }

        return value;
    }
}
