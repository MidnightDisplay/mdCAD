using System.Runtime.InteropServices;

namespace MdCad.Avalonia.Control.Host;

internal static class Win32NativeMethods
{
    private const uint WsChild = 0x40000000;
    private const uint WsVisible = 0x10000000;
    private const uint WsClipSiblings = 0x04000000;
    private const uint WsClipChildren = 0x02000000;

    public delegate bool EnumWindowsProc(IntPtr hwnd, IntPtr lParam);

    public static IntPtr CreatePlaceholderWindow(IntPtr parentHwnd)
    {
        TryGetClientSize(parentHwnd, out int width, out int height);
        if (width <= 0)
        {
            width = 640;
        }

        if (height <= 0)
        {
            height = 480;
        }

        IntPtr hwnd = CreateWindowExW(
            0,
            "STATIC",
            string.Empty,
            WsChild | WsVisible | WsClipSiblings | WsClipChildren,
            0,
            0,
            width,
            height,
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

    public static bool TryGetClientSize(IntPtr hwnd, out int width, out int height)
    {
        width = 0;
        height = 0;
        if (hwnd == IntPtr.Zero)
        {
            return false;
        }

        if (!GetClientRect(hwnd, out RECT rect))
        {
            return false;
        }

        width = rect.Right - rect.Left;
        height = rect.Bottom - rect.Top;
        return true;
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

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    private static extern bool GetClientRect(IntPtr hWnd, out RECT lpRect);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern bool EnumChildWindows(IntPtr hWndParent, EnumWindowsProc lpEnumFunc, IntPtr lParam);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern uint GetWindowThreadProcessId(IntPtr hWnd, out uint lpdwProcessId);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool IsWindow(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    public static extern IntPtr GetParent(IntPtr hWnd);

    [DllImport("user32.dll", SetLastError = true)]
    [return: MarshalAs(UnmanagedType.Bool)]
    public static extern bool MoveWindow(IntPtr hWnd, int x, int y, int nWidth, int nHeight, [MarshalAs(UnmanagedType.Bool)] bool bRepaint);

    [StructLayout(LayoutKind.Sequential)]
    private struct RECT
    {
        public int Left;
        public int Top;
        public int Right;
        public int Bottom;
    }
}
