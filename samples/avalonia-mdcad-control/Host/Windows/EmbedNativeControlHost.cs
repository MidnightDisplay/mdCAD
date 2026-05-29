using Avalonia.Controls;
using Avalonia.Platform;

using MdCad.Embed.Core.Windows;

namespace MdCad.Avalonia.Control.Host;

internal sealed class EmbedNativeControlHost : NativeControlHost
{
    public event Action<IntPtr>? PlaceholderHandleReady;

    public IntPtr PlaceholderHandle { get; private set; }

    public void ClearPlaceholderHandle()
    {
        PlaceholderHandle = IntPtr.Zero;
    }

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
