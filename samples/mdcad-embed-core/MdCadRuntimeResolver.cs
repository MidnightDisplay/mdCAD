using System.IO;

namespace MdCad.Embed.Core;

public static class MdCadRuntimeResolver
{
    public static MdCadRuntimePaths Resolve()
    {
        return ResolveFromBaseDirectory(AppContext.BaseDirectory);
    }

    public static MdCadRuntimePaths ResolveFromBaseDirectory(string baseDirectory)
    {
        MdCadRuntimePaths runtime = MdCadRuntimePaths.CreateFromBaseDirectory(baseDirectory);
        if (!Directory.Exists(runtime.RuntimeRoot))
        {
            throw new InvalidOperationException($"mdCAD runtime folder is missing: {runtime.RuntimeRoot}");
        }

        if (!File.Exists(runtime.ExecutablePath))
        {
            throw new InvalidOperationException($"mdCAD runtime executable is missing: {runtime.ExecutablePath}");
        }

        return runtime;
    }
}
