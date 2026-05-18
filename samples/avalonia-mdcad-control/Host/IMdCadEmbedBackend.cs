using System.Threading;
using System.Threading.Tasks;

using AvaloniaControl = Avalonia.Controls.Control;

namespace MdCad.Avalonia.Control.Host;

internal interface IMdCadEmbedBackend
{
    AvaloniaControl Surface { get; }

    bool CanStartSession { get; }

    IntPtr PlaceholderHandle { get; }

    bool HasActiveSession { get; }

    event Action? StateChanged;

    event Action<string>? UnexpectedSessionLoss;

    Task StartSessionAsync(MdCadLaunchSnapshot snapshot, CancellationToken cancellationToken);

    Task StopSessionAsync(CancellationToken cancellationToken);

    Task RecreateSurfaceAsync(CancellationToken cancellationToken);
}
