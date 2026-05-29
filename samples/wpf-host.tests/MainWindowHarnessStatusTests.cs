using System.Reflection;
using System.Threading;

using MdCad.Embed.Core;

namespace WpfHost.Tests;

public sealed class MainWindowHarnessStatusTests
{
    [Fact]
    public async Task HarnessStatus_MirrorsObservedUnexpectedSessionLossDetailVerbatim()
    {
        const string detail = "Embedded runtime failure: access violation (exception=0xC0000005). See mdcad-embed-crash.log";

        await RunStaAsync(() =>
        {
            MainWindow window = new(new HostLaunchOptions(
                MdCadPresentationMode.Diagnostic,
                HostJsonlScenario.Empty,
                autoStart: false,
                requestLiveRefresh: false,
                viewportOnlyStartupMode: false,
                parseError: null));

            MethodInfo handler = typeof(MainWindow).GetMethod(
                "OnEmbeddedControlUnexpectedSessionLoss",
                BindingFlags.Instance | BindingFlags.NonPublic)
                ?? throw new InvalidOperationException("Missing unexpected-session-loss relay handler.");
            MethodInfo buildHarnessStatus = typeof(MainWindow).GetMethod(
                "BuildHarnessStatus",
                BindingFlags.Instance | BindingFlags.NonPublic)
                ?? throw new InvalidOperationException("Missing BuildHarnessStatus method.");

            handler.Invoke(window, [detail]);
            string status = Assert.IsType<string>(buildHarnessStatus.Invoke(window, null));

            Assert.Contains(detail, status, StringComparison.Ordinal);
            Assert.Contains("viewer state remains control-managed", status, StringComparison.Ordinal);
        });
    }

    private static Task RunStaAsync(Action action)
    {
        TaskCompletionSource completion = new(TaskCreationOptions.RunContinuationsAsynchronously);
        Thread thread = new(() =>
        {
            try
            {
                action();
                completion.SetResult();
            }
            catch (Exception ex)
            {
                completion.SetException(ex);
            }
        });

        thread.SetApartmentState(ApartmentState.STA);
        thread.IsBackground = true;
        thread.Start();
        return completion.Task;
    }
}
