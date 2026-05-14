using Avalonia;
using Avalonia.Controls.ApplicationLifetimes;
using Avalonia.Markup.Xaml;

namespace AvaloniaHost;

public partial class App : Application
{
    public static HostLaunchOptions StartupOptions { get; set; } = HostLaunchOptions.Parse(Array.Empty<string>());

    public override void Initialize()
    {
        AvaloniaXamlLoader.Load(this);
    }

    public override void OnFrameworkInitializationCompleted()
    {
        if (ApplicationLifetime is IClassicDesktopStyleApplicationLifetime desktop)
        {
            desktop.MainWindow = new MainWindow(StartupOptions);
        }

        base.OnFrameworkInitializationCompleted();
    }
}
