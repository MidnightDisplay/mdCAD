using System.IO;

namespace MdCad.Embed.Core.Tests;

public sealed class MdCadRuntimePathsTests
{
    [Fact]
    public void CreateFromBaseDirectory_UsesCanonicalRuntimeLayout()
    {
        string baseDirectory = Path.Combine(
            Path.GetTempPath(),
            $"mdcad-runtime-paths-{Guid.NewGuid():N}");

        MdCadRuntimePaths runtime = MdCadRuntimePaths.CreateFromBaseDirectory(baseDirectory);

        Assert.Equal(Path.Combine(baseDirectory, "mdcad-runtime"), runtime.RuntimeRoot);
        Assert.Equal(Path.Combine(baseDirectory, "mdcad-runtime", "mdCAD.exe"), runtime.ExecutablePath);
        Assert.Equal(Path.Combine(baseDirectory, "mdcad-runtime", "imgui.embedded.ini"), runtime.EmbeddedIniPath);
    }
}
