using System.IO;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control.Tests;

public sealed class MdCadLaunchSnapshotTests
{
    [Theory]
    [InlineData(null)]
    [InlineData("")]
    [InlineData("   ")]
    public void Create_EmptyJsonlRequest_OmitsLaunchPathAndWarning(string? requestedJsonlPath)
    {
        MdCadLaunchSnapshot snapshot = MdCadLaunchSnapshot.Create(
            requestedJsonlPath,
            startupLiveRefreshEnabled: true);

        Assert.Null(snapshot.RequestedJsonlPath);
        Assert.Null(snapshot.LaunchJsonlPath);
        Assert.Null(snapshot.WarningText);
        Assert.False(snapshot.ShouldPassJsonlArgument);
        Assert.False(snapshot.ShouldPassLiveRefreshArgument);
    }

    [Fact]
    public void Create_RelativeJsonlRequest_WarnsAndOmitsLaunchPath()
    {
        MdCadLaunchSnapshot snapshot = MdCadLaunchSnapshot.Create(
            @"relative\sample.jsonl",
            startupLiveRefreshEnabled: true);

        Assert.Equal(@"relative\sample.jsonl", snapshot.RequestedJsonlPath);
        Assert.Null(snapshot.LaunchJsonlPath);
        Assert.Equal(
            "Requested JSONL path must be absolute: relative\\sample.jsonl",
            snapshot.WarningText);
        Assert.False(snapshot.ShouldPassJsonlArgument);
        Assert.False(snapshot.ShouldPassLiveRefreshArgument);
    }

    [Fact]
    public void Create_AbsoluteMissingJsonlRequest_PreservesLaunchPathAndWarns()
    {
        string missingJsonlPath = Path.Combine(Path.GetTempPath(), $"{Guid.NewGuid():N}.jsonl");

        MdCadLaunchSnapshot snapshot = MdCadLaunchSnapshot.Create(
            missingJsonlPath,
            startupLiveRefreshEnabled: true);

        Assert.Equal(missingJsonlPath, snapshot.RequestedJsonlPath);
        Assert.Equal(missingJsonlPath, snapshot.LaunchJsonlPath);
        Assert.Equal(
            $"Requested JSONL file is missing or unreadable: {missingJsonlPath}",
            snapshot.WarningText);
        Assert.True(snapshot.ShouldPassJsonlArgument);
        Assert.True(snapshot.ShouldPassLiveRefreshArgument);
    }

    [Fact]
    public void Create_DefaultViewportOnlyStartupMode_RemainsFalse()
    {
        MdCadLaunchSnapshot snapshot = MdCadLaunchSnapshot.Create(
            null,
            startupLiveRefreshEnabled: false,
            viewportOnlyStartupMode: false);

        Assert.False(snapshot.ViewportOnlyStartupMode);
    }

    [Fact]
    public void Create_WhenViewportOnlyStartupModeRequested_PreservesIntent()
    {
        MdCadLaunchSnapshot snapshot = MdCadLaunchSnapshot.Create(
            null,
            startupLiveRefreshEnabled: false,
            viewportOnlyStartupMode: true);

        Assert.True(snapshot.ViewportOnlyStartupMode);
        Assert.False(snapshot.ShouldPassJsonlArgument);
        Assert.False(snapshot.ShouldPassLiveRefreshArgument);
    }
}
