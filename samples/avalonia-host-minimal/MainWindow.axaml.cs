using Avalonia.Controls;
using Avalonia.Markup.Xaml;

using AvaloniaHostMinimal.ViewModels;

namespace AvaloniaHostMinimal;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        DataContext = new MainWindowViewModel();
    }

    private void InitializeComponent()
    {
        AvaloniaXamlLoader.Load(this);
    }
}
