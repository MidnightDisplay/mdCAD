using System.IO;

namespace MdCad.Embed.Core.Tests;

public sealed class MdCadRuntimeResolverTests
{
    [Fact]
    public void ResolveFromBaseDirectory_ResolvesCopiedRuntimeFolderAndEmbeddedIni()
    {
        string baseDirectory = CreateTempDirectory();
        try
        {
            string runtimeRoot = Path.Combine(baseDirectory, "mdcad-runtime");
            Directory.CreateDirectory(runtimeRoot);
            string executablePath = Path.Combine(runtimeRoot, "mdCAD.exe");
            string embeddedIniPath = Path.Combine(runtimeRoot, "imgui.embedded.ini");
            File.WriteAllText(executablePath, "stub");
            File.WriteAllText(embeddedIniPath, "ini");

            MdCadRuntimePaths runtime = MdCadRuntimeResolver.ResolveFromBaseDirectory(baseDirectory);

            Assert.Equal(executablePath, runtime.ExecutablePath);
            Assert.Equal(runtimeRoot, runtime.RuntimeRoot);
            Assert.Equal(embeddedIniPath, runtime.EmbeddedIniPath);
        }
        finally
        {
            Directory.Delete(baseDirectory, recursive: true);
        }
    }

    [Fact]
    public void ResolveFromBaseDirectory_MissingRuntimeFolder_Throws()
    {
        string baseDirectory = CreateTempDirectory();
        try
        {
            InvalidOperationException ex = Assert.Throws<InvalidOperationException>(
                () => MdCadRuntimeResolver.ResolveFromBaseDirectory(baseDirectory));

            Assert.Contains("mdCAD runtime folder is missing", ex.Message);
            Assert.Contains(Path.Combine(baseDirectory, "mdcad-runtime"), ex.Message);
        }
        finally
        {
            Directory.Delete(baseDirectory, recursive: true);
        }
    }

    [Fact]
    public void ResolveFromBaseDirectory_MissingExecutable_Throws()
    {
        string baseDirectory = CreateTempDirectory();
        try
        {
            string runtimeRoot = Path.Combine(baseDirectory, "mdcad-runtime");
            Directory.CreateDirectory(runtimeRoot);

            InvalidOperationException ex = Assert.Throws<InvalidOperationException>(
                () => MdCadRuntimeResolver.ResolveFromBaseDirectory(baseDirectory));

            Assert.Contains("mdCAD runtime executable is missing", ex.Message);
            Assert.Contains(Path.Combine(runtimeRoot, "mdCAD.exe"), ex.Message);
        }
        finally
        {
            Directory.Delete(baseDirectory, recursive: true);
        }
    }

    private static string CreateTempDirectory()
    {
        string path = Path.Combine(Path.GetTempPath(), $"mdcad-runtime-tests-{Guid.NewGuid():N}");
        Directory.CreateDirectory(path);
        return path;
    }
}
