namespace MdCad.Embed.Core.Tests;

public sealed class MdCadUnsupportedRuntimeTests
{
    [Fact]
    public void PresentationMode_ExposesAvaloniaParityNames()
    {
        string[] names = Enum.GetNames<MdCadPresentationMode>();

        Assert.Equal(["Sealed", "Diagnostic"], names);
        Assert.Equal(0, (int)MdCadPresentationMode.Sealed);
        Assert.Equal(1, (int)MdCadPresentationMode.Diagnostic);
    }

    [Fact]
    public void UnsupportedRuntimeMessage_StaysTruthfulAndCanonical()
    {
        Assert.Equal(
            "This host/control is valid, but embedded mdCAD viewing is Windows-only and will not launch on the current platform.",
            MdCadUnsupportedRuntime.UnsupportedRuntimeMessage);
    }
}
