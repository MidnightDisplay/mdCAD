using System.Threading;
using System.Threading.Tasks;
using System.Windows;

using MdCad.Embed.Core;

namespace MdCad.Wpf.Control.Host;

internal interface IMdCadEmbedBackend
{
    FrameworkElement Surface { get; }

    bool CanStartSession { get; }

    string? StartBlockedReason { get; }

    IntPtr PlaceholderHandle { get; }

    bool HasActiveSession { get; }

    event Action? StateChanged;

    event Action<string>? UnexpectedSessionLoss;

    Task StartSessionAsync(MdCadLaunchSnapshot snapshot, CancellationToken cancellationToken);

    Task StopSessionAsync(CancellationToken cancellationToken);

    Task RecreateSurfaceAsync(CancellationToken cancellationToken);
}
