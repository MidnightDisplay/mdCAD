using System;
using System.Runtime.InteropServices;

using Avalonia.Controls;
using Avalonia.Markup.Xaml;
using Avalonia.Platform;

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

        _embedSurface.PlaceholderHandleReady += OnPlaceholderHandleReady;

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

        _statusTextBlock.Text = StatusWaitingForChildAttach;
        _failureTextBlock.Text = BuildScaffoldMessage(hwnd);
        _modeTextBlock.Text = $"Harness mode: {_options.TestModeToken} (reserved states: {StatusAttached}, {StatusTimeoutFailure})";
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
}
