using System.Threading;
using System.Threading.Tasks;

using MdCad.Avalonia.Control.Host;
using MdCad.Avalonia.Control.Host.Windows;

namespace MdCad.Avalonia.Control.Tests;

public sealed class UnsupportedMdCadEmbedBackendTests
{
    [Fact]
    public void BackendFactory_SelectsUnsupportedBackend_WhenPlatformProbeFalse()
    {
        IMdCadEmbedBackend backend = CreateBackend();

        Assert.IsType<UnsupportedMdCadEmbedBackend>(backend);
        Assert.Equal(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage, backend.StartBlockedReason);
        Assert.False(backend.CanStartSession);
        Assert.False(backend.HasActiveSession);
        Assert.Equal(IntPtr.Zero, backend.PlaceholderHandle);
    }

    [Fact]
    public async Task StartSessionAsync_WhenUnsupported_ThrowsImmediately_WithCanonicalMessage()
    {
        IMdCadEmbedBackend backend = CreateBackend();
        MdCadLaunchSnapshot snapshot = MdCadLaunchSnapshot.Create(@"C:\missing\phase51.jsonl", startupLiveRefreshEnabled: true);

        PlatformNotSupportedException ex = await Assert.ThrowsAsync<PlatformNotSupportedException>(
            () => backend.StartSessionAsync(snapshot, CancellationToken.None));

        Assert.Equal(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage, ex.Message);
        Assert.Equal(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage, backend.StartBlockedReason);
        Assert.False(backend.HasActiveSession);
    }

    [Fact]
    public async Task StopAndRecreateSurface_WhenUnsupported_AreSafeNoOps()
    {
        IMdCadEmbedBackend backend = CreateBackend();

        await backend.StopSessionAsync(CancellationToken.None);
        await backend.RecreateSurfaceAsync(CancellationToken.None);

        Assert.Equal(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage, backend.StartBlockedReason);
        Assert.False(backend.CanStartSession);
        Assert.False(backend.HasActiveSession);
        Assert.Equal(IntPtr.Zero, backend.PlaceholderHandle);
    }

    private static IMdCadEmbedBackend CreateBackend()
    {
        MdCadLaunchSnapshot snapshot = MdCadLaunchSnapshot.Create(null, startupLiveRefreshEnabled: false);
        return MdCadEmbedBackendFactory.Create(
            initialSnapshot: snapshot,
            setLaunchStatus: _ => { },
            setAttachStatus: _ => { },
            updatePreLaunchStatus: _ => { },
            updatePostLaunchStatus: _ => { },
            setFailureStatus: _ => { },
            setLaunchWarning: _ => { },
            updateControlState: () => { },
            captureLaunchSnapshot: () => snapshot,
            isWindows: () => false);
    }
}
