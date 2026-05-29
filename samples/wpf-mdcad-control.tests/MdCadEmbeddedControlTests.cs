using System.IO;
using System.Reflection;
using System.Threading;
using System.Windows.Controls;

using System.Windows;

using MdCad.Embed.Core;
using MdCad.Wpf.Control;
using MdCad.Wpf.Control.Host;

namespace MdCad.Wpf.Control.Tests;

public sealed class MdCadEmbeddedControlTests : IClassFixture<WpfStaThreadFixture>
{
    private readonly WpfStaThreadFixture _sta;

    public MdCadEmbeddedControlTests(WpfStaThreadFixture sta)
    {
        _sta = sta;
    }

    [Fact]
    public async Task Control_ExposesAvaloniaParitySurface()
    {
        await _sta.RunAsync(async () =>
        {
            MdCadEmbeddedControl control = new();

            Assert.Equal(ApartmentState.STA, Thread.CurrentThread.GetApartmentState());
            Assert.Same(MdCadEmbeddedControl.JsonlPathProperty, GetDependencyProperty(nameof(MdCadEmbeddedControl.JsonlPathProperty)));
            Assert.Same(MdCadEmbeddedControl.StartupLiveRefreshEnabledProperty, GetDependencyProperty(nameof(MdCadEmbeddedControl.StartupLiveRefreshEnabledProperty)));
            Assert.Same(MdCadEmbeddedControl.ViewportOnlyStartupModeProperty, GetDependencyProperty(nameof(MdCadEmbeddedControl.ViewportOnlyStartupModeProperty)));
            Assert.Same(MdCadEmbeddedControl.AutoStartProperty, GetDependencyProperty(nameof(MdCadEmbeddedControl.AutoStartProperty)));
            Assert.Same(MdCadEmbeddedControl.PresentationModeProperty, GetDependencyProperty(nameof(MdCadEmbeddedControl.PresentationModeProperty)));

            control.JsonlPath = @"C:\phase56\model.jsonl";
            control.StartupLiveRefreshEnabled = true;
            control.ViewportOnlyStartupMode = true;
            control.AutoStart = false;
            control.PresentationMode = MdCadPresentationMode.Diagnostic;

            await control.StartAsync();
            await control.StopAsync();

            Assert.Equal(@"C:\phase56\model.jsonl", control.JsonlPath);
            Assert.True(control.StartupLiveRefreshEnabled);
            Assert.True(control.ViewportOnlyStartupMode);
            Assert.False(control.AutoStart);
            Assert.Equal(MdCadPresentationMode.Diagnostic, control.PresentationMode);
        });
    }

    [Fact]
    public async Task Control_UpdatesLaunchSnapshotFromParityProperties()
    {
        await _sta.RunAsync(async () =>
        {
            MdCadEmbeddedControl control = new()
            {
                JsonlPath = @"C:\phase56\snapshot.jsonl",
                StartupLiveRefreshEnabled = true,
                ViewportOnlyStartupMode = true,
            };

            await control.StartAsync();

            MdCadLaunchSnapshot snapshot = GetLastLaunchSnapshot(control);

            Assert.Equal(@"C:\phase56\snapshot.jsonl", snapshot.RequestedJsonlPath);
            Assert.True(snapshot.StartupLiveRefreshEnabled);
            Assert.True(snapshot.ViewportOnlyStartupMode);
        });
    }

    [Fact]
    public async Task StartStopAndLaunchChanges_DriveCoordinatorThroughBackend()
    {
        await _sta.RunAsync(async () =>
        {
            string firstJsonlPath = CreateReadableJsonlPath();
            string secondJsonlPath = CreateReadableJsonlPath();

            try
            {
                RecordingBackend backend = new();
                MdCadEmbeddedControl control = new(backend);
                control.AttachForTesting();
                control.JsonlPath = firstJsonlPath;

                await control.StartAsync();

                MdCadLaunchSnapshot initialSnapshot = Assert.Single(backend.StartedSnapshots);
                Assert.Equal(firstJsonlPath, initialSnapshot.LaunchJsonlPath);
                Assert.False(initialSnapshot.ShouldPassLiveRefreshArgument);
                Assert.False(initialSnapshot.ViewportOnlyStartupMode);

                control.JsonlPath = secondJsonlPath;
                control.StartupLiveRefreshEnabled = true;
                control.ViewportOnlyStartupMode = true;

                await control.RequestReconcileForTestingAsync();

                Assert.Equal(1, backend.StopCallCount);
                Assert.Equal(1, backend.RecreateSurfaceCallCount);
                Assert.Equal(2, backend.StartedSnapshots.Count);

                MdCadLaunchSnapshot updatedSnapshot = backend.StartedSnapshots[1];
                Assert.Equal(secondJsonlPath, updatedSnapshot.LaunchJsonlPath);
                Assert.True(updatedSnapshot.ShouldPassLiveRefreshArgument);
                Assert.True(updatedSnapshot.ViewportOnlyStartupMode);

                await control.StopAsync();

                Assert.Equal(2, backend.StopCallCount);
            }
            finally
            {
                File.Delete(firstJsonlPath);
                File.Delete(secondJsonlPath);
            }
        });
    }

    [Fact]
    public async Task Control_RelaysUnexpectedSessionLossAndKeepsWarningTextControlOwned()
    {
        await _sta.RunAsync(async () =>
        {
            RecordingBackend backend = new();
            MdCadEmbeddedControl control = new(backend);
            control.AttachForTesting();

            string? relayedDetail = null;
            control.UnexpectedSessionLoss += detail => relayedDetail = detail;

            const string detail = "Embedded runtime failure: access violation (exception=0xC0000005). See mdcad-embed-crash.log";
            InvokeUnexpectedSessionLoss(control, detail);
            await Task.Yield();

            Assert.Equal(detail, relayedDetail);
            Assert.Equal(detail, GetWarningText(control));
            Assert.Equal($"detail: {detail}", GetTextBlock(control, "_failureTextBlock").Text);
        });
    }

    private static DependencyProperty GetDependencyProperty(string propertyName)
    {
        FieldInfo field = typeof(MdCadEmbeddedControl).GetField(
            propertyName,
            BindingFlags.Public | BindingFlags.Static)
            ?? throw new InvalidOperationException($"Missing dependency property field '{propertyName}'.");

        return Assert.IsType<DependencyProperty>(field.GetValue(null));
    }

    private static MdCadLaunchSnapshot GetLastLaunchSnapshot(MdCadEmbeddedControl control)
    {
        FieldInfo field = typeof(MdCadEmbeddedControl).GetField(
            "_lastLaunchSnapshot",
            BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing _lastLaunchSnapshot field.");

        return Assert.IsType<MdCadLaunchSnapshot>(field.GetValue(control));
    }

    private static string CreateReadableJsonlPath()
    {
        string path = Path.Combine(Path.GetTempPath(), $"mdcad-wpf-control-{Guid.NewGuid():N}.jsonl");
        File.WriteAllText(path, "{}");
        return path;
    }

    private static string? GetWarningText(MdCadEmbeddedControl control)
    {
        FieldInfo field = typeof(MdCadEmbeddedControl).GetField(
            "_warningText",
            BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing _warningText field.");

        return (string?)field.GetValue(control);
    }

    private static TextBlock GetTextBlock(MdCadEmbeddedControl control, string fieldName)
    {
        FieldInfo field = typeof(MdCadEmbeddedControl).GetField(
            fieldName,
            BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException($"Missing field '{fieldName}'.");

        return Assert.IsType<TextBlock>(field.GetValue(control));
    }

    private static void InvokeUnexpectedSessionLoss(MdCadEmbeddedControl control, string detail)
    {
        MethodInfo method = typeof(MdCadEmbeddedControl).GetMethod(
            "OnBackendUnexpectedSessionLoss",
            BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing OnBackendUnexpectedSessionLoss method.");

        method.Invoke(control, [detail]);
    }

    private sealed class RecordingBackend : IMdCadEmbedBackend
    {
        private Action<string>? _unexpectedSessionLoss;

        public RecordingBackend()
        {
            Surface = new Border();
        }

        public FrameworkElement Surface { get; }

        public bool CanStartSession => true;

        public string? StartBlockedReason => null;

        public IntPtr PlaceholderHandle => new(0x1234);

        public bool HasActiveSession { get; private set; }

        public int StopCallCount { get; private set; }

        public int RecreateSurfaceCallCount { get; private set; }

        public List<MdCadLaunchSnapshot> StartedSnapshots { get; } = [];

        public event Action? StateChanged;

        public event Action<string>? UnexpectedSessionLoss
        {
            add => _unexpectedSessionLoss += value;
            remove => _unexpectedSessionLoss -= value;
        }

        public Task StartSessionAsync(MdCadLaunchSnapshot snapshot, CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            StartedSnapshots.Add(snapshot);
            HasActiveSession = true;
            StateChanged?.Invoke();
            return Task.CompletedTask;
        }

        public Task StopSessionAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            StopCallCount++;
            HasActiveSession = false;
            StateChanged?.Invoke();
            return Task.CompletedTask;
        }

        public Task RecreateSurfaceAsync(CancellationToken cancellationToken)
        {
            cancellationToken.ThrowIfCancellationRequested();
            RecreateSurfaceCallCount++;
            StateChanged?.Invoke();
            return Task.CompletedTask;
        }

    }
}
