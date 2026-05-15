# mdCAD Avalonia Control Quick Start

1. Target your Avalonia host project to `net10.0-windows10.0.19041.0`.

   The reusable control stays Windows-only because the embedding seam depends on the Win32 child-HWND contract.

2. Add a project reference to `samples\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj`.

   ```xml
   <ItemGroup>
     <ProjectReference Include="..\avalonia-mdcad-control\MdCad.Avalonia.Control.csproj" />
   </ItemGroup>
   ```

3. Add the control namespace to your XAML.

   ```xml
   xmlns:mdcad="clr-namespace:MdCad.Avalonia.Control;assembly=MdCad.Avalonia.Control"
   ```

4. Expose the startup JSONL path from a viewmodel property.

   ```csharp
   public sealed class MainWindowViewModel
   {
       public string JsonlPath { get; } =
           @"C:\Users\RodionRadchenko\source\repos\GeoMate\artifacts\repl\geo-mate-model.jsonl";
   }
   ```

5. Set the window `DataContext` to that viewmodel.

   ```csharp
   public partial class MainWindow : Window
   {
       public MainWindow()
       {
           InitializeComponent();
           DataContext = new MainWindowViewModel();
       }
   }
   ```

6. Drop `MdCadEmbeddedControl` into your layout and bind `JsonlPath`.

   ```xml
   <Grid Margin="12">
     <mdcad:MdCadEmbeddedControl PresentationMode="Sealed"
                                 AutoStart="True"
                                 JsonlPath="{Binding JsonlPath}" />
   </Grid>
   ```

7. Build and run your app. The control copies its pinned `mdcad-runtime\` bundle into the app output automatically, launches mdCAD in sealed mode, and auto-loads the bound absolute JSONL path on startup.

8. For a working reference, open `samples\avalonia-host-minimal\`. It is the smallest in-repo net10 Windows Avalonia host that binds a hardcoded viewmodel path into the control.
