using System.Reflection;

using Avalonia.Controls;

using MdCad.Avalonia.Control.Host;
using SharedMdCadLaunchSnapshot = MdCad.Embed.Core.MdCadLaunchSnapshot;
using SharedMdCadSessionCoordinator = MdCad.Embed.Core.MdCadSessionCoordinator;

namespace MdCad.Avalonia.Control.Tests;

public sealed class MdCadEmbeddedControlTests
{
    [Fact]
    public void PublicApi_SurfaceRemainsStable()
    {
        MdCadEmbeddedControl control = new();

        Assert.True(control.AutoStart);
        Assert.Equal(MdCadPresentationMode.Sealed, control.PresentationMode);

        AssertPublicProperty<string?>(nameof(MdCadEmbeddedControl.JsonlPath));
        AssertPublicProperty<bool>(nameof(MdCadEmbeddedControl.StartupLiveRefreshEnabled));
        AssertPublicProperty<bool>(nameof(MdCadEmbeddedControl.ViewportOnlyStartupMode));
        AssertPublicProperty<bool>(nameof(MdCadEmbeddedControl.AutoStart));
        AssertPublicProperty<MdCadPresentationMode>(nameof(MdCadEmbeddedControl.PresentationMode));
        AssertPublicGetterOnlyProperty<string?>(nameof(MdCadEmbeddedControl.UnexpectedSessionLossDetail));
        AssertPublicEvent(nameof(MdCadEmbeddedControl.UnexpectedSessionLossChanged), typeof(Action<string?>));
        AssertPublicMethod(nameof(MdCadEmbeddedControl.StartAsync));
        AssertPublicMethod(nameof(MdCadEmbeddedControl.StopAsync));
    }

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

        SharedMdCadLaunchSnapshot snapshot = GetLastLaunchSnapshot(control);
        Assert.True(snapshot.ViewportOnlyStartupMode);
    }

    [Fact]
    public void LaunchSettingsChanges_UseSharedCoreSnapshotAndCoordinator()
    {
        MdCadEmbeddedControl control = new();

        control.AttachForTesting();
        control.JsonlPath = @"C:\phase56\parity.jsonl";
        control.StartupLiveRefreshEnabled = true;
        control.ViewportOnlyStartupMode = true;

        FieldInfo snapshotField = GetRequiredField("_lastLaunchSnapshot");
        FieldInfo coordinatorField = GetRequiredField("_sessionCoordinator");

        Assert.Equal(typeof(SharedMdCadLaunchSnapshot), snapshotField.FieldType);
        Assert.Equal(typeof(SharedMdCadSessionCoordinator), coordinatorField.FieldType);

        SharedMdCadLaunchSnapshot snapshot = Assert.IsType<SharedMdCadLaunchSnapshot>(snapshotField.GetValue(control));
        Assert.Equal(@"C:\phase56\parity.jsonl", snapshot.RequestedJsonlPath);
        Assert.True(snapshot.StartupLiveRefreshEnabled);
        Assert.True(snapshot.ViewportOnlyStartupMode);
    }

    [Fact]
    public void UnexpectedSessionLossDetail_IsStickyAndObservableUntilTheUserChangesLaunchSettings()
    {
        MdCadEmbeddedControl control = new();
        string? observedDetail = null;
        control.UnexpectedSessionLossChanged += detail => observedDetail = detail;

        const string detail =
            "Embedded runtime failure: mdCAD terminated with an unhandled embedded exception (exception=0xC0000005). See mdcad-embed-crash.log";

        InvokePrivateInstanceMethod(control, "OnBackendUnexpectedSessionLoss", detail);
        InvokePrivateInstanceMethod(control, "SetLaunchWarning", null);

        Border warningSurface = GetControl<Border>(control, "WarningSurface");
        TextBlock warningText = GetControl<TextBlock>(control, "WarningTextBlock");

        Assert.Equal(detail, control.UnexpectedSessionLossDetail);
        Assert.Equal(detail, observedDetail);
        Assert.True(warningSurface.IsVisible);
        Assert.Equal(detail, warningText.Text);

        control.JsonlPath = @"C:\phase57\retry.jsonl";

        Assert.Null(control.UnexpectedSessionLossDetail);
        Assert.NotEqual(detail, warningText.Text);
    }

    private static void AssertPublicProperty<TProperty>(string propertyName)
    {
        PropertyInfo property = typeof(MdCadEmbeddedControl).GetProperty(propertyName)
            ?? throw new InvalidOperationException($"Missing public property '{propertyName}'.");

        Assert.Equal(typeof(TProperty), property.PropertyType);
        Assert.NotNull(property.GetMethod);
        Assert.NotNull(property.SetMethod);
    }

    private static void AssertPublicGetterOnlyProperty<TProperty>(string propertyName)
    {
        PropertyInfo property = typeof(MdCadEmbeddedControl).GetProperty(propertyName)
            ?? throw new InvalidOperationException($"Missing public property '{propertyName}'.");

        Assert.Equal(typeof(TProperty), property.PropertyType);
        Assert.NotNull(property.GetMethod);
        Assert.Null(property.SetMethod);
    }

    private static void AssertPublicEvent(string eventName, Type eventHandlerType)
    {
        EventInfo eventInfo = typeof(MdCadEmbeddedControl).GetEvent(eventName)
            ?? throw new InvalidOperationException($"Missing public event '{eventName}'.");

        Assert.Equal(eventHandlerType, eventInfo.EventHandlerType);
    }

    private static void AssertPublicMethod(string methodName)
    {
        MethodInfo method = typeof(MdCadEmbeddedControl).GetMethod(methodName)
            ?? throw new InvalidOperationException($"Missing public method '{methodName}'.");

        Assert.Equal(typeof(Task), method.ReturnType);
    }

    private static FieldInfo GetRequiredField(string fieldName)
    {
        return typeof(MdCadEmbeddedControl).GetField(fieldName, BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException($"Missing {fieldName} field.");
    }

    private static TControl GetControl<TControl>(MdCadEmbeddedControl control, string name)
        where TControl : global::Avalonia.Controls.Control
    {
        return control.FindControl<TControl>(name)
            ?? throw new InvalidOperationException($"Missing control '{name}'.");
    }

    private static SharedMdCadLaunchSnapshot GetLastLaunchSnapshot(MdCadEmbeddedControl control)
    {
        FieldInfo field = GetRequiredField("_lastLaunchSnapshot");

        return Assert.IsType<SharedMdCadLaunchSnapshot>(field.GetValue(control));
    }

    private static object? InvokePrivateInstanceMethod(MdCadEmbeddedControl control, string methodName, params object?[] args)
    {
        MethodInfo method = typeof(MdCadEmbeddedControl).GetMethod(methodName, BindingFlags.Instance | BindingFlags.NonPublic)
            ?? throw new InvalidOperationException($"Missing {methodName} method.");

        return method.Invoke(control, args);
    }
}
