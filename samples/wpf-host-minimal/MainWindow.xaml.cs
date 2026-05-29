using System.Windows;

using WpfHostMinimal.ViewModels;

namespace WpfHostMinimal;

public partial class MainWindow : Window
{
    public MainWindow()
    {
        InitializeComponent();
        DataContext = new MainWindowViewModel();
    }
}
