using Avalonia;

namespace AvaloniaHost;

internal static class Program
{
    [STAThread]
    public static void Main(string[] args)
    {
        App.StartupOptions = HostLaunchOptions.Parse(args);
        BuildAvaloniaApp().StartWithClassicDesktopLifetime(args);
    }

    public static AppBuilder BuildAvaloniaApp()
        => AppBuilder.Configure<App>()
            .UsePlatformDetect()
            .LogToTrace();
}
