using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control.Tests;

public sealed class MdCadSessionCoordinatorTests
{
    [Fact]
    public async Task AutoStart_WaitsForSurfaceReadinessBeforeLaunching()
    {
        TestCoordinatorHarness harness = new()
        {
            AutoStart = true,
            SurfaceReady = false,
            PlaceholderHandle = new IntPtr(0x100),
            Snapshot = MdCadLaunchSnapshot.Create(null, startupLiveRefreshEnabled: false),
        };

        await using MdCadSessionCoordinator coordinator = harness.CreateCoordinator();

        await coordinator.RequestReconcileAsync();
        Assert.Empty(harness.StartedSnapshots);

        harness.SurfaceReady = true;
        await coordinator.RequestReconcileAsync();

        Assert.Single(harness.StartedSnapshots);
        Assert.True(coordinator.IsSessionRunning);
    }

    [Fact]
    public async Task AutoStartFalse_StaysIdleUntilExplicitStart()
    {
        TestCoordinatorHarness harness = new()
        {
            AutoStart = false,
            SurfaceReady = true,
            PlaceholderHandle = new IntPtr(0x100),
            Snapshot = MdCadLaunchSnapshot.Create(null, startupLiveRefreshEnabled: false),
        };

        await using MdCadSessionCoordinator coordinator = harness.CreateCoordinator();

        await coordinator.RequestReconcileAsync();
        Assert.Empty(harness.StartedSnapshots);

        await coordinator.StartAsync();

        Assert.Single(harness.StartedSnapshots);
        Assert.True(coordinator.IsSessionRunning);
    }

    [Fact]
    public async Task LaunchAffectingChanges_CoalesceToOneRelaunchUsingNewestSnapshot()
    {
        TestCoordinatorHarness harness = new()
        {
            AutoStart = true,
            SurfaceReady = true,
            PlaceholderHandle = new IntPtr(0x100),
            Snapshot = MdCadLaunchSnapshot.Create(
                Path.Combine(Path.GetTempPath(), "phase48-first.jsonl"),
                startupLiveRefreshEnabled: false),
        };

        await using MdCadSessionCoordinator coordinator = harness.CreateCoordinator();
        await coordinator.RequestReconcileAsync();

        harness.Snapshot = MdCadLaunchSnapshot.Create(
            Path.Combine(Path.GetTempPath(), "phase48-second.jsonl"),
            startupLiveRefreshEnabled: false);
        Task firstReconcile = coordinator.RequestReconcileAsync();

        harness.Snapshot = MdCadLaunchSnapshot.Create(
            Path.Combine(Path.GetTempPath(), "phase48-third.jsonl"),
            startupLiveRefreshEnabled: true);
        Task secondReconcile = coordinator.RequestReconcileAsync();

        await Task.WhenAll(firstReconcile, secondReconcile);

        Assert.Equal(1, harness.StopCount);
        Assert.Equal(1, harness.RecreateCount);
        Assert.Equal(2, harness.StartedSnapshots.Count);
        Assert.Equal(harness.Snapshot, harness.StartedSnapshots[^1]);
    }

    [Fact]
    public async Task StopAsync_TearsDownStateAndRecreatesPlaceholderBeforeNextStart()
    {
        TestCoordinatorHarness harness = new()
        {
            AutoStart = false,
            SurfaceReady = true,
            PlaceholderHandle = new IntPtr(0x100),
            Snapshot = MdCadLaunchSnapshot.Create(null, startupLiveRefreshEnabled: false),
        };

        await using MdCadSessionCoordinator coordinator = harness.CreateCoordinator();
        await coordinator.StartAsync();

        await coordinator.StopAsync();

        Assert.Equal(1, harness.StopCount);
        Assert.Equal(1, harness.RecreateCount);
        Assert.False(coordinator.IsSessionRunning);

        await coordinator.StartAsync();

        Assert.Equal(2, harness.StartedSnapshots.Count);
        Assert.True(coordinator.IsSessionRunning);
    }

    private sealed class TestCoordinatorHarness
    {
        public bool AutoStart { get; set; }

        public bool SurfaceReady { get; set; }

        public IntPtr PlaceholderHandle { get; set; }

        public MdCadLaunchSnapshot Snapshot { get; set; }

        public string? WarningText { get; private set; }

        public int StopCount { get; private set; }

        public int RecreateCount { get; private set; }

        public List<MdCadLaunchSnapshot> StartedSnapshots { get; } = [];

        public MdCadSessionCoordinator CreateCoordinator()
        {
            return new MdCadSessionCoordinator(
                captureSnapshot: () => Snapshot,
                canStartSession: () => SurfaceReady && PlaceholderHandle != IntPtr.Zero,
                getPlaceholderHandle: () => PlaceholderHandle,
                getAutoStart: () => AutoStart,
                applyWarning: warningText => WarningText = warningText,
                startSessionAsync: (snapshot, placeholderHandle, _) =>
                {
                    StartedSnapshots.Add(snapshot);
                    PlaceholderHandle = placeholderHandle;
                    return Task.CompletedTask;
                },
                stopSessionAsync: _ =>
                {
                    StopCount++;
                    return Task.CompletedTask;
                },
                recreateSurfaceAsync: _ =>
                {
                    RecreateCount++;
                    PlaceholderHandle = new IntPtr(0x100 + RecreateCount);
                    SurfaceReady = true;
                    return Task.CompletedTask;
                });
        }
    }
}
