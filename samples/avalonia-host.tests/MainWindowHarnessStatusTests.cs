using System.Reflection;

using AvaloniaHost;

namespace AvaloniaHost.Tests;

public sealed class MainWindowHarnessStatusTests
{
    [Fact]
    public void UnexpectedSessionLossDetail_IsMirroredVerbatimInHarnessStatus()
    {
        const string detail =
            "Embedded runtime failure: mdCAD terminated with an unhandled embedded exception (exception=0xC0000005). See mdcad-embed-crash.log";

        string status = InvokeComposeHarnessStatus(detail);

        Assert.Contains("mode=diagnostic", status);
        Assert.Contains($"observed detail: {detail}", status);
    }

    private static string InvokeComposeHarnessStatus(string detail)
    {
        MethodInfo method = typeof(MainWindow).GetMethod(
            "ComposeHarnessStatus",
            BindingFlags.Static | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing ComposeHarnessStatus.");

        return Assert.IsType<string>(method.Invoke(null, ["diagnostic", "false", "off", "jsonl=none", null, detail]));
    }
}
