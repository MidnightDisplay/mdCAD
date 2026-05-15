using System.IO;

namespace MdCad.Avalonia.Control.Host;

internal readonly record struct MdCadRuntimePaths(string ExecutablePath, string RuntimeRoot);

internal static class MdCadRuntimeResolver
{
    private const string RuntimeDirectoryName = "mdcad-runtime";
    private const string ExecutableName = "mdCAD.exe";

    public static MdCadRuntimePaths Resolve()
    {
        string runtimeRoot = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, RuntimeDirectoryName));
        if (!Directory.Exists(runtimeRoot))
        {
            throw new InvalidOperationException($"mdCAD runtime folder is missing: {runtimeRoot}");
        }

        string executablePath = Path.Combine(runtimeRoot, ExecutableName);
        if (!File.Exists(executablePath))
        {
            throw new InvalidOperationException($"mdCAD runtime executable is missing: {executablePath}");
        }

        return new MdCadRuntimePaths(executablePath, runtimeRoot);
    }
}
