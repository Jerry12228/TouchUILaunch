using Avalonia;
using Avalonia.Animation;
using Avalonia.Animation.Easings;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Media;

namespace Nte.LoginHost;

internal sealed class SlidingModeSelector : Border
{
    private readonly Grid track = new();

    private readonly Border thumb = new()
        { CornerRadius = new(8), IsHitTestVisible = false, HorizontalAlignment = HorizontalAlignment.Left };

    private readonly TranslateTransform translation = new();

    private readonly DoubleTransition transition = new()
    {
        Property = TranslateTransform.XProperty, Duration = TimeSpan.FromMilliseconds(180), Easing = new CubicEaseOut()
    };

    private readonly RadioButton second;

    internal SlidingModeSelector(RadioButton first, RadioButton second)
    {
        this.second = second;
        Padding = new(3);
        CornerRadius = new(10);
        this.SetResourceReference(BackgroundProperty, "InputBrush");
        thumb.SetResourceReference(BackgroundProperty, "AccentBrush");
        thumb.RenderTransform = translation;
        translation.Transitions = [transition];
        track.Children.Add(thumb);
        Grid labels = new() { ColumnDefinitions = new("*,*") };
        labels.Children.Add(first);
        Grid.SetColumn(second, 1);
        labels.Children.Add(second);
        track.Children.Add(labels);
        Child = track;
        SizeChanged += (_, _) => Move(false);
        first.IsCheckedChanged += (_, _) => Move(true);
        second.IsCheckedChanged += (_, _) => Move(true);
    }

    private void Move(bool animate)
    {
        double width = track.Bounds.Width / 2;
        if (width <= 0) return;
        thumb.Width = width;
        transition.Duration = animate && IsLoaded && NativeUiSettings.ClientAreaAnimation
            ? TimeSpan.FromMilliseconds(180)
            : TimeSpan.Zero;
        translation.X = second.IsChecked == true ? width : 0;
    }
}
