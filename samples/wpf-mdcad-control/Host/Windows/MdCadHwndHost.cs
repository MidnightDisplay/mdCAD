using System.Runtime.InteropServices;
using System.Windows.Interop;

using MdCad.Embed.Core.Windows;

namespace MdCad.Wpf.Control.Host.Windows;

internal sealed class MdCadHwndHost : HwndHost
{
    public event Action<IntPtr>? PlaceholderHandleReady;

    public IntPtr PlaceholderHandle { get; private set; }

    public void ClearPlaceholderHandle()
    {
        PlaceholderHandle = IntPtr.Zero;
    }

    protected override HandleRef BuildWindowCore(HandleRef hwndParent)
    {
        if (!OperatingSystem.IsWindows())
        {
            throw new PlatformNotSupportedException("mdCAD WPF embedding requires Windows.");
        }

        if (hwndParent.Handle == IntPtr.Zero)
        {
            throw new InvalidOperationException("Expected a valid Win32 parent handle for HwndHost.");
        }

        PlaceholderHandle = Win32NativeMethods.CreatePlaceholderWindow(hwndParent.Handle);
        PlaceholderHandleReady?.Invoke(PlaceholderHandle);
        return new HandleRef(this, PlaceholderHandle);
    }

    protected override void DestroyWindowCore(HandleRef hwnd)
    {
        if (OperatingSystem.IsWindows() && hwnd.Handle != IntPtr.Zero && Win32NativeMethods.IsWindow(hwnd.Handle))
        {
            Win32NativeMethods.DestroyWindow(hwnd.Handle);
        }

        if (PlaceholderHandle == hwnd.Handle)
        {
            PlaceholderHandle = IntPtr.Zero;
        }
    }
}
