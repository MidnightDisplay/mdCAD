using System.IO;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control.Tests;

public sealed class MdCadRuntimeResolverTests
{
    [Fact]
    public void ResolveFromBaseDirectory_ResolvesCopiedRuntimeFolder()
    {
        string baseDirectory = CreateTempDirectory();
        try
        {
            string runtimeRoot = Path.Combine(baseDirectory, "mdcad-runtime");
            Directory.CreateDirectory(runtimeRoot);
            string executablePath = Path.Combine(runtimeRoot, "mdCAD.exe");
            File.WriteAllText(executablePath, "stub");

            MdCadRuntimePaths runtime = MdCadRuntimeResolver.ResolveFromBaseDirectory(baseDirectory);

            Assert.Equal(executablePath, runtime.ExecutablePath);
            Assert.Equal(runtimeRoot, runtime.RuntimeRoot);
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
