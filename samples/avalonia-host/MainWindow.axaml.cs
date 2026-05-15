using System.IO;

using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Markup.Xaml;

using MdCad.Avalonia.Control;

namespace AvaloniaHost;

public enum HostJsonlScenario
{
    BundledExample,
    Empty,
    MissingFile,
    UnreadableDirectory,
}

public sealed class HostLaunchOptions
{
    public MdCadPresentationMode PresentationMode { get; }
    public HostJsonlScenario JsonlScenario { get; }
    public bool AutoStart { get; }
    public bool RequestLiveRefresh { get; }
    public string? ParseError { get; }

    private HostLaunchOptions(
        MdCadPresentationMode presentationMode,
        HostJsonlScenario jsonlScenario,
        bool autoStart,
        bool requestLiveRefresh,
        string? parseError)
    {
        PresentationMode = presentationMode;
        JsonlScenario = jsonlScenario;
        AutoStart = autoStart;
        RequestLiveRefresh = requestLiveRefresh;
        ParseError = parseError;
    }

    public static HostLaunchOptions Parse(string[] args)
    {
        MdCadPresentationMode presentationMode = MdCadPresentationMode.Diagnostic;
        HostJsonlScenario jsonlScenario = HostJsonlScenario.BundledExample;
        bool autoStart = true;
        bool requestLiveRefresh = false;
        string? parseError = null;

        for (int i = 0; i < args.Length; ++i)
        {
            string arg = args[i];
            if (string.Equals(arg, "--presentation-mode", StringComparison.Ordinal))
            {
                if ((i + 1) >= args.Length)
                {
                    parseError = "Missing value for --presentation-mode";
                    break;
                }

                string modeToken = args[++i];
                if (string.Equals(modeToken, "sealed", StringComparison.OrdinalIgnoreCase))
                {
                    presentationMode = MdCadPresentationMode.Sealed;
                }
                else if (string.Equals(modeToken, "diagnostic", StringComparison.OrdinalIgnoreCase))
                {
                    presentationMode = MdCadPresentationMode.Diagnostic;
                }
                else
                {
                    parseError = $"Unsupported --presentation-mode value: {modeToken}";
                    break;
                }

                continue;
            }

            if (string.Equals(arg, "--jsonl-scenario", StringComparison.Ordinal))
            {
                if ((i + 1) >= args.Length)
                {
                    parseError = "Missing value for --jsonl-scenario";
                    break;
                }

                string scenarioToken = args[++i];
                if (string.Equals(scenarioToken, "example", StringComparison.OrdinalIgnoreCase))
                {
                    jsonlScenario = HostJsonlScenario.BundledExample;
                }
                else if (string.Equals(scenarioToken, "empty", StringComparison.OrdinalIgnoreCase))
                {
                    jsonlScenario = HostJsonlScenario.Empty;
                }
                else if (string.Equals(scenarioToken, "missing", StringComparison.OrdinalIgnoreCase))
                {
                    jsonlScenario = HostJsonlScenario.MissingFile;
                }
                else if (string.Equals(scenarioToken, "unreadable", StringComparison.OrdinalIgnoreCase))
                {
                    jsonlScenario = HostJsonlScenario.UnreadableDirectory;
                }
                else
                {
                    parseError = $"Unsupported --jsonl-scenario value: {scenarioToken}";
                    break;
                }

                continue;
            }

            if (string.Equals(arg, "--auto-start", StringComparison.Ordinal))
            {
                if ((i + 1) >= args.Length)
                {
                    parseError = "Missing value for --auto-start";
                    break;
                }

                string autoStartToken = args[++i];
                if (bool.TryParse(autoStartToken, out bool parsedAutoStart))
                {
                    autoStart = parsedAutoStart;
                }
                else
                {
                    parseError = $"Unsupported --auto-start value: {autoStartToken}";
                    break;
                }

                continue;
            }

            if (string.Equals(arg, "--jsonl-live-refresh", StringComparison.Ordinal))
            {
                requestLiveRefresh = true;
                continue;
            }

            parseError = $"Unsupported argument: {arg}";
            break;
        }

        return new HostLaunchOptions(
            presentationMode,
            jsonlScenario,
            autoStart,
            requestLiveRefresh,
            parseError);
    }
}

public partial class MainWindow : Window
{
    private const string BundledExampleFileName = "sample-host-proof.jsonl";
    private const string RuntimeExecutableName = "mdCAD.exe";

    private readonly HostLaunchOptions _options;
    private readonly ComboBox _presentationModeComboBox;
    private readonly CheckBox _autoStartCheckBox;
    private readonly CheckBox _liveRefreshCheckBox;
    private readonly ComboBox _jsonlScenarioComboBox;
    private readonly Button _startViewerButton;
    private readonly Button _stopViewerButton;
    private readonly TextBlock _runtimeStatusTextBlock;
    private readonly TextBlock _harnessStatusTextBlock;
    private readonly MdCadEmbeddedControl _embeddedControl;
    private bool _isInitializing;
    private string? _currentJsonlPath;
    private string _currentJsonlStatus = string.Empty;
    private string? _lastActionNote;

    public MainWindow()
        : this(HostLaunchOptions.Parse(Array.Empty<string>()))
    {
    }

    public MainWindow(HostLaunchOptions options)
    {
        _options = options;
        _isInitializing = true;
        InitializeComponent();

        _presentationModeComboBox = this.FindControl<ComboBox>("PresentationModeComboBox")
            ?? throw new InvalidOperationException("Missing PresentationModeComboBox.");
        _autoStartCheckBox = this.FindControl<CheckBox>("AutoStartCheckBox")
            ?? throw new InvalidOperationException("Missing AutoStartCheckBox.");
        _liveRefreshCheckBox = this.FindControl<CheckBox>("LiveRefreshCheckBox")
            ?? throw new InvalidOperationException("Missing LiveRefreshCheckBox.");
        _jsonlScenarioComboBox = this.FindControl<ComboBox>("JsonlScenarioComboBox")
            ?? throw new InvalidOperationException("Missing JsonlScenarioComboBox.");
        _startViewerButton = this.FindControl<Button>("StartViewerButton")
            ?? throw new InvalidOperationException("Missing StartViewerButton.");
        _stopViewerButton = this.FindControl<Button>("StopViewerButton")
            ?? throw new InvalidOperationException("Missing StopViewerButton.");
        _runtimeStatusTextBlock = this.FindControl<TextBlock>("RuntimeStatusTextBlock")
            ?? throw new InvalidOperationException("Missing RuntimeStatusTextBlock.");
        _harnessStatusTextBlock = this.FindControl<TextBlock>("HarnessStatusTextBlock")
            ?? throw new InvalidOperationException("Missing HarnessStatusTextBlock.");
        _embeddedControl = this.FindControl<MdCadEmbeddedControl>("EmbeddedControl")
            ?? throw new InvalidOperationException("Missing EmbeddedControl.");

        ApplyStartupOptions();
        _isInitializing = false;

        if (!string.IsNullOrWhiteSpace(_options.ParseError))
        {
            DisableHarnessForParseError();
            return;
        }

        ApplyHarnessSettings();
    }

    private void InitializeComponent()
    {
        AvaloniaXamlLoader.Load(this);
    }

    private void ApplyStartupOptions()
    {
        _presentationModeComboBox.SelectedIndex = _options.PresentationMode == MdCadPresentationMode.Sealed ? 0 : 1;
        _autoStartCheckBox.IsChecked = _options.AutoStart;
        _liveRefreshCheckBox.IsChecked = _options.RequestLiveRefresh;
        _jsonlScenarioComboBox.SelectedIndex = _options.JsonlScenario switch
        {
            HostJsonlScenario.BundledExample => 0,
            HostJsonlScenario.Empty => 1,
            HostJsonlScenario.MissingFile => 2,
            HostJsonlScenario.UnreadableDirectory => 3,
            _ => 0,
        };
    }

    private void DisableHarnessForParseError()
    {
        _presentationModeComboBox.IsEnabled = false;
        _autoStartCheckBox.IsEnabled = false;
        _liveRefreshCheckBox.IsEnabled = false;
        _jsonlScenarioComboBox.IsEnabled = false;
        _startViewerButton.IsEnabled = false;
        _stopViewerButton.IsEnabled = false;
        _runtimeStatusTextBlock.Text = BuildRuntimeStatus();
        _harnessStatusTextBlock.Text = $"harness: {_options.ParseError}";
    }

    private void ApplyHarnessSettings()
    {
        _currentJsonlPath = ResolveJsonlPath(GetSelectedJsonlScenario(), out _currentJsonlStatus);
        _embeddedControl.PresentationMode = GetSelectedPresentationMode();
        _embeddedControl.AutoStart = _autoStartCheckBox.IsChecked == true;
        _embeddedControl.StartupLiveRefreshEnabled = _liveRefreshCheckBox.IsChecked == true;
        _embeddedControl.JsonlPath = _currentJsonlPath;
        _runtimeStatusTextBlock.Text = BuildRuntimeStatus();
        _harnessStatusTextBlock.Text = BuildHarnessStatus();
    }

    private MdCadPresentationMode GetSelectedPresentationMode()
    {
        return _presentationModeComboBox.SelectedIndex == 0
            ? MdCadPresentationMode.Sealed
            : MdCadPresentationMode.Diagnostic;
    }

    private HostJsonlScenario GetSelectedJsonlScenario()
    {
        return _jsonlScenarioComboBox.SelectedIndex switch
        {
            1 => HostJsonlScenario.Empty,
            2 => HostJsonlScenario.MissingFile,
            3 => HostJsonlScenario.UnreadableDirectory,
            _ => HostJsonlScenario.BundledExample,
        };
    }

    private string BuildRuntimeStatus()
    {
        string executablePath = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "mdcad-runtime", RuntimeExecutableName));
        return File.Exists(executablePath)
            ? $"runtime: copied bundle present -> {executablePath}"
            : $"runtime: copied bundle missing -> {executablePath}";
    }

    private string BuildHarnessStatus()
    {
        string modeText = GetSelectedPresentationMode() == MdCadPresentationMode.Sealed ? "sealed" : "diagnostic";
        string autoStartText = _autoStartCheckBox.IsChecked == true ? "true" : "false";
        string liveRefreshText = _liveRefreshCheckBox.IsChecked == true ? "requested" : "off";
        string note = string.IsNullOrWhiteSpace(_lastActionNote) ? string.Empty : $" | action: {_lastActionNote}";
        return $"harness: mode={modeText}; AutoStart={autoStartText}; live refresh={liveRefreshText}; {_currentJsonlStatus}{note}";
    }

    private static string? ResolveJsonlPath(HostJsonlScenario scenario, out string statusText)
    {
        string exampleDirectory = Path.GetFullPath(Path.Combine(AppContext.BaseDirectory, "resources", "examples"));
        string bundledExamplePath = Path.Combine(exampleDirectory, BundledExampleFileName);

        switch (scenario)
        {
            case HostJsonlScenario.Empty:
                statusText = "jsonl: no startup file requested";
                return null;

            case HostJsonlScenario.MissingFile:
                string missingFilePath = Path.Combine(exampleDirectory, "missing-sample-host-proof.jsonl");
                statusText = $"jsonl: missing file requested -> {missingFilePath}";
                return missingFilePath;

            case HostJsonlScenario.UnreadableDirectory:
                statusText = $"jsonl: unreadable directory requested -> {exampleDirectory}";
                return exampleDirectory;

            default:
                statusText = File.Exists(bundledExamplePath)
                    ? $"jsonl: example resolved -> {bundledExamplePath}"
                    : $"jsonl: example missing -> {bundledExamplePath}";
                return bundledExamplePath;
        }
    }

    private void OnPresentationModeChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private void OnAutoStartToggleChanged(object? sender, RoutedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private void OnLiveRefreshToggleChanged(object? sender, RoutedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private void OnJsonlScenarioChanged(object? sender, SelectionChangedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private async void OnStartViewerClick(object? sender, RoutedEventArgs e)
    {
        try
        {
            await _embeddedControl.StartAsync();
            _lastActionNote = "StartAsync requested";
        }
        catch (Exception ex)
        {
            _lastActionNote = $"StartAsync failed: {ex.Message}";
        }

        _harnessStatusTextBlock.Text = BuildHarnessStatus();
    }

    private async void OnStopViewerClick(object? sender, RoutedEventArgs e)
    {
        try
        {
            await _embeddedControl.StopAsync();
            _lastActionNote = "StopAsync requested";
        }
        catch (Exception ex)
        {
            _lastActionNote = $"StopAsync failed: {ex.Message}";
        }

        _harnessStatusTextBlock.Text = BuildHarnessStatus();
    }
}
