using System.Reflection;

using Avalonia.Controls;

using MdCad.Avalonia.Control.Host;

namespace MdCad.Avalonia.Control.Tests;

public sealed class MdCadEmbeddedControlTests
{
    [Fact]
    public void UnsupportedState_ShowsCanonicalWarningImmediately()
    {
        MdCadEmbeddedControl control = new(isWindowsOverride: () => false);

        control.AttachForTesting();

        Border warningSurface = GetControl<Border>(control, "WarningSurface");
        TextBlock warningText = GetControl<TextBlock>(control, "WarningTextBlock");

        Assert.True(warningSurface.IsVisible);
        Assert.Equal(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage, warningText.Text);
    }

    [Fact]
    public void UnsupportedState_RequestedInputsRemainSecondaryDiagnostics()
    {
        MdCadEmbeddedControl control = new(isWindowsOverride: () => false)
        {
            PresentationMode = MdCadPresentationMode.Diagnostic,
        };

        control.AttachForTesting();
        control.JsonlPath = @"C:\missing\phase51.jsonl";
        control.StartupLiveRefreshEnabled = true;

        TextBlock warningText = GetControl<TextBlock>(control, "WarningTextBlock");
        TextBlock jsonlStatus = GetControl<TextBlock>(control, "JsonlStatusTextBlock");
        TextBlock liveRefreshStatus = GetControl<TextBlock>(control, "LiveRefreshStatusTextBlock");

        Assert.Equal(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage, warningText.Text);
        Assert.Equal(@"jsonl: requested (informational only) -> C:\missing\phase51.jsonl", jsonlStatus.Text);
        Assert.Equal("live refresh: requested (informational only)", liveRefreshStatus.Text);
        Assert.DoesNotContain("missing or unreadable", warningText.Text);
    }

    [Fact]
    public void UnsupportedState_DisablesDiagnosticLaunch()
    {
        MdCadEmbeddedControl control = new(isWindowsOverride: () => false)
        {
            PresentationMode = MdCadPresentationMode.Diagnostic,
        };

        control.AttachForTesting();

        Button startButton = GetControl<Button>(control, "DiagnosticStartButton");
        Button stopButton = GetControl<Button>(control, "DiagnosticStopButton");
        TextBlock launchStatus = GetControl<TextBlock>(control, "LaunchStatusTextBlock");
        TextBlock attachStatus = GetControl<TextBlock>(control, "AttachStatusTextBlock");

        Assert.False(startButton.IsEnabled);
        Assert.False(stopButton.IsEnabled);
        Assert.Equal("launch: unsupported", launchStatus.Text);
        Assert.Equal("attach: unsupported", attachStatus.Text);
    }

    [Fact]
    public void ViewportOnlyStartupMode_UpdatesCapturedLaunchSnapshot()
    {
        MdCadEmbeddedControl control = new();

        control.ViewportOnlyStartupMode = true;

        MdCadLaunchSnapshot snapshot = GetLastLaunchSnapshot(control);
        Assert.True(snapshot.ViewportOnlyStartupMode);
    }

    private static TControl GetControl<TControl>(MdCadEmbeddedControl control, string name)
        where TControl : global::Avalonia.Controls.Control
    {
        return control.FindControl<TControl>(name)
            ?? throw new InvalidOperationException($"Missing control '{name}'.");
    }

    private static MdCadLaunchSnapshot GetLastLaunchSnapshot(MdCadEmbeddedControl control)
    {
        FieldInfo field = typeof(MdCadEmbeddedControl).GetField(
            "_lastLaunchSnapshot",
            BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException("Missing _lastLaunchSnapshot field.");

        return Assert.IsType<MdCadLaunchSnapshot>(field.GetValue(control));
    }
}
