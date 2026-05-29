using System.IO;

namespace MdCad.Embed.Core;

public readonly record struct MdCadRuntimePaths(
    string ExecutablePath,
    string RuntimeRoot,
    string EmbeddedIniPath)
{
    private const string RuntimeDirectoryName = "mdcad-runtime";
    private const string ExecutableName = "mdCAD.exe";
    private const string EmbeddedIniName = "imgui.embedded.ini";

    public static MdCadRuntimePaths CreateFromBaseDirectory(string baseDirectory)
    {
        ArgumentException.ThrowIfNullOrWhiteSpace(baseDirectory);

        string runtimeRoot = Path.GetFullPath(Path.Combine(baseDirectory, RuntimeDirectoryName));
        return new MdCadRuntimePaths(
            ExecutablePath: Path.Combine(runtimeRoot, ExecutableName),
            RuntimeRoot: runtimeRoot,
            EmbeddedIniPath: Path.Combine(runtimeRoot, EmbeddedIniName));
    }
}
