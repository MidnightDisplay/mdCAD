using System.IO;
using System.Windows;
using System.Windows.Controls;

using MdCad.Embed.Core;
using MdCad.Wpf.Control;

namespace WpfHost;

public enum HostJsonlScenario
{
    BundledExample,
    Empty,
    MissingFile,
    UnreadableDirectory,
}

public sealed class HostLaunchOptions
{
    public HostLaunchOptions(
        MdCadPresentationMode presentationMode,
        HostJsonlScenario jsonlScenario,
        bool autoStart,
        bool requestLiveRefresh,
        bool viewportOnlyStartupMode,
        string? parseError)
    {
        PresentationMode = presentationMode;
        JsonlScenario = jsonlScenario;
        AutoStart = autoStart;
        RequestLiveRefresh = requestLiveRefresh;
        ViewportOnlyStartupMode = viewportOnlyStartupMode;
        ParseError = parseError;
    }

    public MdCadPresentationMode PresentationMode { get; }

    public HostJsonlScenario JsonlScenario { get; }

    public bool AutoStart { get; }

    public bool RequestLiveRefresh { get; }

    public bool ViewportOnlyStartupMode { get; }

    public string? ParseError { get; }

    public static HostLaunchOptions Parse(string[] args)
    {
        MdCadPresentationMode presentationMode = MdCadPresentationMode.Diagnostic;
        HostJsonlScenario jsonlScenario = HostJsonlScenario.BundledExample;
        bool autoStart = true;
        bool requestLiveRefresh = false;
        bool viewportOnlyStartupMode = false;
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

            if (string.Equals(arg, "--viewport-only", StringComparison.Ordinal))
            {
                viewportOnlyStartupMode = true;
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
            viewportOnlyStartupMode,
            parseError);
    }
}

public partial class MainWindow : Window
{
    private const string BundledExampleFileName = "sample-host-proof.jsonl";
    private const string RuntimeExecutableName = "mdCAD.exe";

    private readonly HostLaunchOptions _options;
    private bool _isInitializing;
    private string? _currentJsonlPath;
    private string _currentJsonlStatus = string.Empty;
    private string? _lastActionNote;
    private string? _lastObservedUnexpectedSessionLossDetail;

    public MainWindow()
        : this(HostLaunchOptions.Parse([]))
    {
    }

    public MainWindow(HostLaunchOptions options)
    {
        _options = options;
        _isInitializing = true;
        InitializeComponent();
        EmbeddedControl.UnexpectedSessionLoss += OnEmbeddedControlUnexpectedSessionLoss;

        ApplyStartupOptions();
        _isInitializing = false;

        if (!string.IsNullOrWhiteSpace(_options.ParseError))
        {
            DisableHarnessForParseError();
            return;
        }

        ApplyHarnessSettings();
    }

    private void ApplyStartupOptions()
    {
        PresentationModeComboBox.SelectedIndex = _options.PresentationMode == MdCadPresentationMode.Sealed ? 0 : 1;
        AutoStartCheckBox.IsChecked = _options.AutoStart;
        LiveRefreshCheckBox.IsChecked = _options.RequestLiveRefresh;
        ViewportOnlyCheckBox.IsChecked = _options.ViewportOnlyStartupMode;
        JsonlScenarioComboBox.SelectedIndex = _options.JsonlScenario switch
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
        PresentationModeComboBox.IsEnabled = false;
        AutoStartCheckBox.IsEnabled = false;
        LiveRefreshCheckBox.IsEnabled = false;
        ViewportOnlyCheckBox.IsEnabled = false;
        JsonlScenarioComboBox.IsEnabled = false;
        StartViewerButton.IsEnabled = false;
        StopViewerButton.IsEnabled = false;
        RuntimeStatusTextBlock.Text = BuildRuntimeStatus();
        HarnessStatusTextBlock.Text = $"harness: {_options.ParseError}";
    }

    private void ApplyHarnessSettings()
    {
        _currentJsonlPath = ResolveJsonlPath(GetSelectedJsonlScenario(), out _currentJsonlStatus);
        EmbeddedControl.PresentationMode = GetSelectedPresentationMode();
        EmbeddedControl.AutoStart = AutoStartCheckBox.IsChecked == true;
        EmbeddedControl.StartupLiveRefreshEnabled = LiveRefreshCheckBox.IsChecked == true;
        EmbeddedControl.ViewportOnlyStartupMode = ViewportOnlyCheckBox.IsChecked == true;
        EmbeddedControl.JsonlPath = _currentJsonlPath;
        RuntimeStatusTextBlock.Text = BuildRuntimeStatus();
        HarnessStatusTextBlock.Text = BuildHarnessStatus();
    }

    private MdCadPresentationMode GetSelectedPresentationMode()
    {
        return PresentationModeComboBox.SelectedIndex == 0
            ? MdCadPresentationMode.Sealed
            : MdCadPresentationMode.Diagnostic;
    }

    private HostJsonlScenario GetSelectedJsonlScenario()
    {
        return JsonlScenarioComboBox.SelectedIndex switch
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
        string autoStartText = AutoStartCheckBox.IsChecked == true ? "true" : "false";
        string liveRefreshText = LiveRefreshCheckBox.IsChecked == true ? "requested" : "off";
        string viewportOnlyText = ViewportOnlyCheckBox.IsChecked == true ? "requested" : "off";
        string note = string.IsNullOrWhiteSpace(_lastActionNote) ? string.Empty : $" | action: {_lastActionNote}";
        string observedDetail = string.IsNullOrWhiteSpace(_lastObservedUnexpectedSessionLossDetail)
            ? string.Empty
            : $" | observed unexpected session loss: {_lastObservedUnexpectedSessionLossDetail}";
        return $"harness: mode={modeText}; AutoStart={autoStartText}; live refresh={liveRefreshText}; viewport only={viewportOnlyText}; {_currentJsonlStatus}; viewer state remains control-managed{observedDetail}{note}";
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
                    ? $"jsonl: bundled example resolved -> {bundledExamplePath}"
                    : $"jsonl: bundled example missing -> {bundledExamplePath}";
                return bundledExamplePath;
        }
    }

    private void OnPresentationModeChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private void OnAutoStartToggleChanged(object sender, RoutedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private void OnLiveRefreshToggleChanged(object sender, RoutedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private void OnViewportOnlyToggleChanged(object sender, RoutedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private void OnJsonlScenarioChanged(object sender, SelectionChangedEventArgs e)
    {
        if (_isInitializing)
        {
            return;
        }

        _lastActionNote = null;
        ApplyHarnessSettings();
    }

    private void OnEmbeddedControlUnexpectedSessionLoss(string detail)
    {
        _lastObservedUnexpectedSessionLossDetail = detail;
        HarnessStatusTextBlock.Text = BuildHarnessStatus();
    }

    private async void OnStartViewerClick(object sender, RoutedEventArgs e)
    {
        try
        {
            await EmbeddedControl.StartAsync();
            _lastActionNote = "host requested StartAsync()";
        }
        catch (Exception ex)
        {
            _lastActionNote = $"host StartAsync() request failed: {ex.Message}";
        }

        HarnessStatusTextBlock.Text = BuildHarnessStatus();
    }

    private async void OnStopViewerClick(object sender, RoutedEventArgs e)
    {
        try
        {
            await EmbeddedControl.StopAsync();
            _lastActionNote = "host requested StopAsync()";
        }
        catch (Exception ex)
        {
            _lastActionNote = $"host StopAsync() request failed: {ex.Message}";
        }

        HarnessStatusTextBlock.Text = BuildHarnessStatus();
    }
}
