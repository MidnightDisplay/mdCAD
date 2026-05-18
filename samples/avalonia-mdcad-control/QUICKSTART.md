# mdCAD Avalonia Control Quick Start

1. Target your Avalonia host project to `net10.0`.

   `MdCad.Avalonia.Control` can be referenced from a plain `net10.0` Avalonia host. The embedded mdCAD viewer itself remains Windows-only at runtime because the actual child-HWND viewer seam depends on the Win32 contract.

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

4. Expose the startup JSONL path from a viewmodel property if you want launch-time import.

   ```csharp
   public sealed class MainWindowViewModel
   {
       public string JsonlPath { get; } = string.Empty;
   }
   ```

   Replace `string.Empty` with your own absolute JSONL path when you want startup import.

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

7. Build and run your app. The control can be referenced from a plain `net10.0` host. On Windows, it copies its pinned `mdcad-runtime\` bundle into the app output, launches mdCAD in sealed mode, and auto-loads the bound absolute JSONL path when `JsonlPath` is set. On unsupported platforms, the host/control remains valid but embedded viewing will not launch, and the control shows the Windows-only warning instead.

8. For a working compile/build reference, open `samples\avalonia-host-minimal\`. It is the smallest in-repo plain `net10.0` Avalonia host that binds `JsonlPath` into the control. For the authoritative Windows runtime proof surface, use `samples\avalonia-host\`.
