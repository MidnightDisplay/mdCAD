using System;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Runtime.InteropServices;

using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Platform;
using Avalonia.Threading;

namespace AvaloniaHost;

public enum EmbedTestMode
{
    Valid,
    InvalidParent,
    DestroyedParent,
}

public sealed class EmbedNativeControlHost : NativeControlHost
{
    public event Action<IntPtr>? PlaceholderHandleReady;

    public IntPtr PlaceholderHandle { get; private set; }

    protected override IPlatformHandle CreateNativeControlCore(IPlatformHandle parent)
    {
        if (!OperatingSystem.IsWindows())
        {
            return base.CreateNativeControlCore(parent);
        }

        IntPtr parentHandle = parent.Handle;
        if (parentHandle == IntPtr.Zero)
        {
            throw new InvalidOperationException("Expected a valid Win32 parent handle for NativeControlHost.");
        }

        PlaceholderHandle = Win32NativeMethods.CreatePlaceholderWindow(parentHandle);
        PlaceholderHandleReady?.Invoke(PlaceholderHandle);
        return new PlatformHandle(PlaceholderHandle, "HWND");
    }

    protected override void DestroyNativeControlCore(IPlatformHandle control)
    {
        if (OperatingSystem.IsWindows() && control.Handle != IntPtr.Zero)
        {
            Win32NativeMethods.DestroyWindow(control.Handle);
            if (PlaceholderHandle == control.Handle)
            {
                PlaceholderHandle = IntPtr.Zero;
            }
            return;
        }

        base.DestroyNativeControlCore(control);
    }
}

public sealed class HostLaunchOptions
{
    public string? MdcadExePath { get; }
    public EmbedTestMode TestMode { get; }
    public string? ParseError { get; }

    public string TestModeToken => TestMode switch
    {
        EmbedTestMode.Valid => "valid",
        EmbedTestMode.InvalidParent => "invalid-parent",
        EmbedTestMode.DestroyedParent => "destroyed-parent",
        _ => "valid",
    };

    private HostLaunchOptions(string? mdcadExePath, EmbedTestMode testMode, string? parseError)
    {
        MdcadExePath = mdcadExePath;
        TestMode = testMode;
        ParseError = parseError;
    }

    public static HostLaunchOptions Parse(string[] args)
    {
        string? mdcadExePath = null;
        EmbedTestMode testMode = EmbedTestMode.Valid;
        string? parseError = null;

        for (int i = 0; i < args.Length; ++i)
        {
            string arg = args[i];
            if (string.Equals(arg, "--mdcad-exe", StringComparison.Ordinal))
            {
                if ((i + 1) >= args.Length || string.IsNullOrWhiteSpace(args[i + 1]))
                {
                    parseError = "Missing value for --mdcad-exe";
                    break;
                }
                mdcadExePath = args[++i];
                continue;
            }

            if (string.Equals(arg, "--embed-test-mode", StringComparison.Ordinal))
            {
                if ((i + 1) >= args.Length || string.IsNullOrWhiteSpace(args[i + 1]))
                {
                    parseError = "Missing value for --embed-test-mode";
                    break;
                }

                string modeToken = args[++i];
                if (string.Equals(modeToken, "valid", StringComparison.Ordinal))
                {
                    testMode = EmbedTestMode.Valid;
                }
                else if (string.Equals(modeToken, "invalid-parent", StringComparison.Ordinal))
                {
                    testMode = EmbedTestMode.InvalidParent;
                }
                else if (string.Equals(modeToken, "destroyed-parent", StringComparison.Ordinal))
                {
                    testMode = EmbedTestMode.DestroyedParent;
                }
                else
                {
                    parseError = $"Unsupported --embed-test-mode value: {modeToken}";
                    break;
                }
            }
        }

        return new HostLaunchOptions(mdcadExePath, testMode, parseError);
    }
}

public partial class MainWindow : Window
{
    private const string StatusLaunching = "launching";
    private const string StatusWaitingForChildAttach = "waiting for child attach";
    private const string StatusAttached = "attached";
    private const string StatusTimeoutFailure = "timeout/failure";

    private readonly HostLaunchOptions _options;
    private readonly EmbedNativeControlHost _embedSurface;
    private readonly TextBlock _statusTextBlock;
    private readonly TextBlock _modeTextBlock;
    private readonly TextBlock _failureTextBlock;
    private readonly DispatcherTimer _attachTimer;
    private Process? _mdcadProcess;
    private IntPtr _launchParentHwnd;
    private IntPtr _attachedChildHwnd;
    private DateTimeOffset _launchStartedAt;
    private bool _launchStarted;
    private string? _capturedFailureLine;

    public MainWindow()
        : this(HostLaunchOptions.Parse(Array.Empty<string>()))
    {
    }

    public MainWindow(HostLaunchOptions options)
    {
        _options = options;
        InitializeComponent();

        _embedSurface = this.FindControl<EmbedNativeControlHost>("EmbedSurface")
            ?? throw new InvalidOperationException("Missing EmbedSurface control.");
        _statusTextBlock = this.FindControl<TextBlock>("StatusTextBlock")
            ?? throw new InvalidOperationException("Missing StatusTextBlock.");
        _modeTextBlock = this.FindControl<TextBlock>("ModeTextBlock")
            ?? throw new InvalidOperationException("Missing ModeTextBlock.");
        _failureTextBlock = this.FindControl<TextBlock>("FailureTextBlock")
            ?? throw new InvalidOperationException("Missing FailureTextBlock.");
        _attachTimer = new DispatcherTimer { Interval = TimeSpan.FromMilliseconds(100) };

        _embedSurface.PlaceholderHandleReady += OnPlaceholderHandleReady;
        _attachTimer.Tick += OnAttachTimerTick;
        Closed += OnWindowClosed;

        _statusTextBlock.Text = StatusLaunching;
        _modeTextBlock.Text = $"Harness mode: {_options.TestModeToken}";
        _failureTextBlock.Text = BuildScaffoldMessage(IntPtr.Zero);

        if (!string.IsNullOrWhiteSpace(_options.ParseError))
        {
            _statusTextBlock.Text = StatusTimeoutFailure;
            _failureTextBlock.Text = _options.ParseError;
        }
    }

    private void InitializeComponent()
    {
        AvaloniaXamlLoader.Load(this);
    }

    private void OnPlaceholderHandleReady(IntPtr hwnd)
    {
        if (!string.IsNullOrWhiteSpace(_options.ParseError))
        {
            _statusTextBlock.Text = StatusTimeoutFailure;
            return;
        }

        if (_launchStarted)
        {
            return;
        }

        _launchStarted = true;
        _launchParentHwnd = PrepareLaunchParentHwnd(hwnd);
        _failureTextBlock.Text = BuildScaffoldMessage(hwnd);
        LaunchEmbeddedViewer();
    }

    private string BuildScaffoldMessage(IntPtr hwnd)
    {
        string pathText = string.IsNullOrWhiteSpace(_options.MdcadExePath)
            ? "mdCAD path: deferred relative resolution"
            : $"mdCAD path override: {_options.MdcadExePath}";
        string hwndText = hwnd == IntPtr.Zero
            ? "Host HWND: pending NativeControlHost creation"
            : $"Host HWND: 0x{hwnd.ToInt64():X}";

        return $"{pathText}{Environment.NewLine}{hwndText}{Environment.NewLine}"
             + "Wave 0 scaffold reserves --mdcad-exe, --embed-test-mode valid|invalid-parent|destroyed-parent, "
             + "and the staged statuses launching, waiting for child attach, attached, timeout/failure.";
    }

    private IntPtr PrepareLaunchParentHwnd(IntPtr placeholderHwnd)
    {
        if (_options.TestMode == EmbedTestMode.InvalidParent)
        {
            return new IntPtr(0x1);
        }

        if (_options.TestMode == EmbedTestMode.DestroyedParent)
        {
            if (placeholderHwnd != IntPtr.Zero)
            {
                Win32NativeMethods.DestroyWindow(placeholderHwnd);
            }
            return placeholderHwnd;
        }

        return placeholderHwnd;
    }

    private void LaunchEmbeddedViewer()
    {
        if (!TryResolveMdcadExecutable(out string? mdCadExePath, out string failureMessage))
        {
            SetFailureStatus(failureMessage);
            return;
        }

        _statusTextBlock.Text = StatusLaunching;
        _failureTextBlock.Text =
            $"mdCAD path: {mdCadExePath}{Environment.NewLine}" +
            $"Parent HWND: 0x{_launchParentHwnd.ToInt64():X}{Environment.NewLine}" +
            $"Mode: {_options.TestModeToken}";

        ProcessStartInfo startInfo = new()
        {
            FileName = mdCadExePath,
            Arguments = $"--embedded --parent-hwnd 0x{_launchParentHwnd.ToInt64():X}",
            UseShellExecute = false,
            RedirectStandardOutput = true,
            RedirectStandardError = true,
            CreateNoWindow = false,
            WorkingDirectory = Path.GetDirectoryName(mdCadExePath) ?? Directory.GetCurrentDirectory(),
        };

        try
        {
            _mdcadProcess = new Process
            {
                StartInfo = startInfo,
                EnableRaisingEvents = true,
            };
            _mdcadProcess.OutputDataReceived += OnChildOutputDataReceived;
            _mdcadProcess.ErrorDataReceived += OnChildOutputDataReceived;
            _mdcadProcess.Exited += OnChildProcessExited;

            if (!_mdcadProcess.Start())
            {
                SetFailureStatus("Failed to start mdCAD.");
                return;
            }
        }
        catch (Win32Exception ex)
        {
            SetFailureStatus($"Failed to start mdCAD: {ex.Message}");
            return;
        }
        catch (InvalidOperationException ex)
        {
            SetFailureStatus($"Failed to start mdCAD: {ex.Message}");
            return;
        }

        _mdcadProcess.BeginOutputReadLine();
        _mdcadProcess.BeginErrorReadLine();

        _launchStartedAt = DateTimeOffset.UtcNow;
        _statusTextBlock.Text = StatusWaitingForChildAttach;
        _modeTextBlock.Text = $"Harness mode: {_options.TestModeToken}";
        _attachTimer.Start();
    }

    private void OnChildOutputDataReceived(object sender, DataReceivedEventArgs e)
    {
        if (string.IsNullOrWhiteSpace(e.Data))
        {
            return;
        }

        if (_capturedFailureLine == null && e.Data.Contains("Embedded startup failed:", StringComparison.Ordinal))
        {
            _capturedFailureLine = e.Data.Trim();
            Dispatcher.UIThread.Post(() =>
            {
                if (_attachedChildHwnd == IntPtr.Zero)
                {
                    _failureTextBlock.Text = _capturedFailureLine;
                }
            });
        }
    }

    private void OnChildProcessExited(object? sender, EventArgs e)
    {
        Dispatcher.UIThread.Post(() =>
        {
            if (_attachedChildHwnd != IntPtr.Zero)
            {
                return;
            }

            int exitCode = _mdcadProcess?.HasExited == true ? _mdcadProcess.ExitCode : -1;
            string detail = _capturedFailureLine
                ?? $"mdCAD exited before child attach (exit code {exitCode}).";
            SetFailureStatus(detail);
        });
    }

    private void OnAttachTimerTick(object? sender, EventArgs e)
    {
        if (_attachedChildHwnd != IntPtr.Zero)
        {
            _attachTimer.Stop();
            return;
        }

        if (_mdcadProcess == null)
        {
            SetFailureStatus("mdCAD process was not started.");
            return;
        }

        if (_mdcadProcess.HasExited)
        {
            int exitCode = _mdcadProcess.ExitCode;
            string detail = _capturedFailureLine
                ?? $"mdCAD exited before child attach (exit code {exitCode}).";
            SetFailureStatus(detail);
            return;
        }

        IntPtr childHwnd = FindChildWindowForProcess(_launchParentHwnd, _mdcadProcess.Id);
        if (childHwnd != IntPtr.Zero)
        {
            _attachedChildHwnd = childHwnd;
            _attachTimer.Stop();
            _statusTextBlock.Text = StatusAttached;
            _failureTextBlock.Text =
                $"Attached child HWND: 0x{childHwnd.ToInt64():X}{Environment.NewLine}" +
                $"Parent HWND: 0x{_launchParentHwnd.ToInt64():X}";
            return;
        }

        if ((DateTimeOffset.UtcNow - _launchStartedAt) >= TimeSpan.FromSeconds(10))
        {
            SetFailureStatus(_capturedFailureLine ?? "Timed out waiting for child attach.");
        }
    }

    private void SetFailureStatus(string detail)
    {
        _attachTimer.Stop();
        _statusTextBlock.Text = StatusTimeoutFailure;
        _failureTextBlock.Text = string.IsNullOrWhiteSpace(_capturedFailureLine)
            ? detail
            : $"{_capturedFailureLine}{Environment.NewLine}{detail}";
    }

    private bool TryResolveMdcadExecutable(out string? mdCadExePath, out string failureMessage)
    {
        if (!string.IsNullOrWhiteSpace(_options.MdcadExePath))
        {
            string overridePath = Path.GetFullPath(_options.MdcadExePath);
            if (File.Exists(overridePath))
            {
                mdCadExePath = overridePath;
                failureMessage = string.Empty;
                return true;
            }

            mdCadExePath = null;
            failureMessage = $"Unable to resolve mdCAD.exe override: {overridePath}";
            return false;
        }

        string repoRoot = FindRepoRoot();
        string[] candidates =
        {
            Path.Combine(repoRoot, "build-vulkan", "bin", "Release", "mdCAD.exe"),
            Path.Combine(repoRoot, "build-vulkan", "bin", "mdCAD.exe"),
        };

        foreach (string candidate in candidates)
        {
            if (File.Exists(candidate))
            {
                mdCadExePath = candidate;
                failureMessage = string.Empty;
                return true;
            }
        }

        mdCadExePath = null;
        failureMessage =
            $"Unable to resolve mdCAD.exe. Checked:{Environment.NewLine}" +
            string.Join(Environment.NewLine, candidates);
        return false;
    }

    private static string FindRepoRoot()
    {
        DirectoryInfo? current = new DirectoryInfo(AppContext.BaseDirectory);
        while (current != null)
        {
            bool hasPlanning = Directory.Exists(Path.Combine(current.FullName, ".planning"));
            bool hasSource = Directory.Exists(Path.Combine(current.FullName, "src"));
            if (hasPlanning && hasSource)
            {
                return current.FullName;
            }

            current = current.Parent;
        }

        return Directory.GetCurrentDirectory();
    }

    private static IntPtr FindChildWindowForProcess(IntPtr parentHwnd, int processId)
    {
        if (!OperatingSystem.IsWindows() || parentHwnd == IntPtr.Zero || !Win32NativeMethods.IsWindow(parentHwnd))
        {
            return IntPtr.Zero;
        }

        IntPtr matchedChild = IntPtr.Zero;
        Win32NativeMethods.EnumChildWindows(parentHwnd, (hwnd, _) =>
        {
            Win32NativeMethods.GetWindowThreadProcessId(hwnd, out uint windowProcessId);
            if (windowProcessId == (uint)processId)
            {
                matchedChild = hwnd;
                return false;
            }

            return true;
        }, IntPtr.Zero);

        return matchedChild;
    }

    private void OnWindowClosed(object? sender, EventArgs e)
    {
        _attachTimer.Stop();

        if (_mdcadProcess != null)
        {
            try
            {
                if (!_mdcadProcess.HasExited)
                {
                    _mdcadProcess.Kill(true);
                }
            }
            catch (InvalidOperationException)
            {
            }

            _mdcadProcess.Dispose();
            _mdcadProcess = null;
        }
    }
}

internal static class Win32NativeMethods
{
    private const uint WsChild = 0x40000000;
    private const uint WsVisible = 0x10000000;
    private const uint WsClipSiblings = 0x04000000;
    private const uint WsClipChildren = 0x02000000;

    public static IntPtr CreatePlaceholderWindow(IntPtr parentHwnd)
    {
        IntPtr hwnd = CreateWindowExW(
            0,
            "STATIC",
            string.Empty,
            WsChild | WsVisible | WsClipSiblings | WsClipChildren,
            0,
            0,
            1,
            1,
            parentHwnd,
            IntPtr.Zero,
            IntPtr.Zero,
            IntPtr.Zero);

        if (hwnd == IntPtr.Zero)
        {
            throw new InvalidOperationException("Failed to create a placeholder HWND for NativeControlHost.");
        }

        return hwnd;
    }

    public delegate bool EnumWindowsProc(IntPtr hwnd, IntPtr lParam);

    [DllImport("user32.dll", CharSet = CharSet.Unicode, SetLastError = true)]
    private static extern IntPtr CreateWindowExW(
        uint dwExStyle,
        string lpClassName,
        string lpWindowName,
        uint dwStyle,
        int x,
        int y,
        int nWidth,
        int nHeight,
        IntPtr hWndParent,
        IntPtr hMenu,
        IntPtr hInstance,
        IntPtr lpParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool DestroyWindow(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsWindow(IntPtr hWnd);
}
