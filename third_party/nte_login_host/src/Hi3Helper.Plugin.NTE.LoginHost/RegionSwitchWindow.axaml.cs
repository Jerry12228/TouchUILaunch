using Nte.LoginHost.ViewModels;
using Avalonia.Markup.Xaml;
using Avalonia.Threading;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;

namespace Nte.LoginHost;

internal partial class RegionSwitchWindow : Window
{
    private readonly RegionSwitchViewModel viewModel;
    private bool closeWhenIdle;
    internal RegionSwitchWindow(RegionSwitchViewModel viewModel)
    {
        AvaloniaXamlLoader.Load(this); this.viewModel = viewModel; DataContext = viewModel;
        Loaded += async (_, _) => await viewModel.CheckCommand.ExecuteAsync();
        Closing += OnClosing;
        Closed += (_, _) => viewModel.Dispose();
        viewModel.PropertyChanged += (_, args) =>
        {
            if (args.PropertyName == nameof(viewModel.IsBusy) && !viewModel.IsBusy && closeWhenIdle) Dispatcher.UIThread.Post(Close);
        };
    }
    private void OnClosing(object? sender, WindowClosingEventArgs args)
    {
        if (!viewModel.IsBusy) return;
        args.Cancel = true; closeWhenIdle = true; viewModel.CancelCommand.Execute(null);
    }
}
