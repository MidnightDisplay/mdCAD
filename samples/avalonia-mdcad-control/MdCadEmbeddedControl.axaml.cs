using System.Threading.Tasks;

using Avalonia;
using Avalonia.Controls;
using Avalonia.Markup.Xaml;

namespace MdCad.Avalonia.Control;

public partial class MdCadEmbeddedControl : UserControl
{
    public static readonly StyledProperty<string?> JsonlPathProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, string?>(nameof(JsonlPath));

    public static readonly StyledProperty<bool> StartupLiveRefreshEnabledProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(StartupLiveRefreshEnabled));

    public static readonly StyledProperty<bool> AutoStartProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, bool>(nameof(AutoStart), defaultValue: true);

    public static readonly StyledProperty<MdCadPresentationMode> PresentationModeProperty =
        AvaloniaProperty.Register<MdCadEmbeddedControl, MdCadPresentationMode>(
            nameof(PresentationMode),
            defaultValue: MdCadPresentationMode.Sealed);

    private readonly ContentControl _embedSurfaceContainer;
    private readonly Border _warningSurface;
    private readonly TextBlock _warningTextBlock;

    public MdCadEmbeddedControl()
    {
        InitializeComponent();

        _embedSurfaceContainer = this.FindControl<ContentControl>("EmbedSurfaceContainer")
            ?? throw new InvalidOperationException("Missing EmbedSurfaceContainer.");
        _warningSurface = this.FindControl<Border>("WarningSurface")
            ?? throw new InvalidOperationException("Missing WarningSurface.");
        _warningTextBlock = this.FindControl<TextBlock>("WarningTextBlock")
            ?? throw new InvalidOperationException("Missing WarningTextBlock.");

        UpdateWarningSurface(null);
    }

    public string? JsonlPath
    {
        get => GetValue(JsonlPathProperty);
        set => SetValue(JsonlPathProperty, value);
    }

    public bool StartupLiveRefreshEnabled
    {
        get => GetValue(StartupLiveRefreshEnabledProperty);
        set => SetValue(StartupLiveRefreshEnabledProperty, value);
    }

    public bool AutoStart
    {
        get => GetValue(AutoStartProperty);
        set => SetValue(AutoStartProperty, value);
    }

    public MdCadPresentationMode PresentationMode
    {
        get => GetValue(PresentationModeProperty);
        set => SetValue(PresentationModeProperty, value);
    }

    public Task StartAsync()
    {
        return Task.CompletedTask;
    }

    public Task StopAsync()
    {
        return Task.CompletedTask;
    }

    internal void SetLaunchWarning(string? warningText)
    {
        UpdateWarningSurface(warningText);
    }

    private void InitializeComponent()
    {
        AvaloniaXamlLoader.Load(this);
    }

    private void UpdateWarningSurface(string? warningText)
    {
        bool hasWarning = !string.IsNullOrWhiteSpace(warningText);
        _warningSurface.IsVisible = hasWarning;
        _warningTextBlock.Text = hasWarning ? warningText! : string.Empty;
    }
}
