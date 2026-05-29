using System.Windows;

namespace WpfHost;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        MainWindow window = new(HostLaunchOptions.Parse(e.Args));
        MainWindow = window;
        window.Show();
    }
}
