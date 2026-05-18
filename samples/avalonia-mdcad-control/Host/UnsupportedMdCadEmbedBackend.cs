using System.Threading;
using System.Threading.Tasks;

using Avalonia.Controls;
using AvaloniaControl = Avalonia.Controls.Control;

namespace MdCad.Avalonia.Control.Host;

internal sealed class UnsupportedMdCadEmbedBackend : IMdCadEmbedBackend
{
    private readonly Border _surface = new();

    public AvaloniaControl Surface => _surface;

    public bool CanStartSession => false;

    public string? StartBlockedReason => MdCadUnsupportedRuntime.UnsupportedRuntimeMessage;

    public IntPtr PlaceholderHandle => IntPtr.Zero;

    public bool HasActiveSession => false;

    public event Action? StateChanged
    {
        add { }
        remove { }
    }

    public event Action<string>? UnexpectedSessionLoss
    {
        add { }
        remove { }
    }

    public Task StartSessionAsync(MdCadLaunchSnapshot snapshot, CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        throw new PlatformNotSupportedException(MdCadUnsupportedRuntime.UnsupportedRuntimeMessage);
    }

    public Task StopSessionAsync(CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.CompletedTask;
    }

    public Task RecreateSurfaceAsync(CancellationToken cancellationToken)
    {
        cancellationToken.ThrowIfCancellationRequested();
        return Task.CompletedTask;
    }
}
