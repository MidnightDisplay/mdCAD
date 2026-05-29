using System.Reflection;

using Avalonia.Controls;

using AvaloniaHost;

using MdCad.Avalonia.Control;

namespace AvaloniaHost.Tests;

public sealed class MainWindowHarnessStatusTests
{
    [Fact]
    public void UnexpectedSessionLossDetail_IsMirroredVerbatimInHarnessStatus()
    {
        MainWindow window = new();
        MdCadEmbeddedControl control = window.FindControl<MdCadEmbeddedControl>("EmbeddedControl")
            ?? throw new InvalidOperationException("Missing EmbeddedControl.");
        TextBlock harnessStatus = window.FindControl<TextBlock>("HarnessStatusTextBlock")
            ?? throw new InvalidOperationException("Missing HarnessStatusTextBlock.");
        TextBlock warningText = control.FindControl<TextBlock>("WarningTextBlock")
            ?? throw new InvalidOperationException("Missing WarningTextBlock.");
        const string detail =
            "Embedded runtime failure: mdCAD terminated with an unhandled embedded exception (exception=0xC0000005). See mdcad-embed-crash.log";

        InvokePrivateUnexpectedSessionLoss(control, detail);

        Assert.Contains($"observed detail: {detail}", harnessStatus.Text);
        Assert.Equal(detail, warningText.Text);
        Assert.Equal(detail, control.UnexpectedSessionLossDetail);
    }

    private static void InvokePrivateUnexpectedSessionLoss(MdCadEmbeddedControl control, string detail)
    {
        MethodInfo method = typeof(MdCadEmbeddedControl).GetMethod(
            "OnBackendUnexpectedSessionLoss",
            BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing OnBackendUnexpectedSessionLoss.");

        method.Invoke(control, [detail]);
    }
}
