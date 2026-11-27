using System.Diagnostics;
using Avalonia;
using Avalonia.Automation;
using Avalonia.Controls;
using Avalonia.Controls.Documents;
using Avalonia.Layout;
using Avalonia.Media;
using Avalonia.Styling;
using Nte.Login;
using Nte.LoginHost.Localization;

namespace Nte.LoginHost;

internal sealed class GlobalAgreementBlock : Grid
{
    internal GlobalAgreementBlock(CheckBox checkbox, ControlTheme style, NteOverseasRegion? region = null)
    {
        Margin = new(0, 6, 0, 0);
        ColumnDefinitions.Add(new() { Width = new(24) });
        ColumnDefinitions.Add(new());
        checkbox.Theme = style;
        checkbox.Content = null;
        checkbox.VerticalAlignment = VerticalAlignment.Top;
        AutomationProperties.SetName(checkbox, LoginText.Get("Agreement"));
        AutomationProperties.SetAutomationId(checkbox, "agreement");
        Children.Add(checkbox);
        TextBlock text = new()
            { FontSize = 12, LineHeight = 20, TextWrapping = TextWrapping.Wrap, Margin = new(8, 4, 0, 0) };
        string format = LoginText.Get("AgreementInline");
        while (format.Length > 0)
        {
            int terms = format.IndexOf("{0}", StringComparison.Ordinal),
                privacy = format.IndexOf("{1}", StringComparison.Ordinal);
            int position = terms < 0 ? privacy : privacy < 0 ? terms : Math.Min(terms, privacy);
            if (position < 0)
            {
                text.Inlines!.Add(new Run(format));
                break;
            }

            if (position > 0) text.Inlines!.Add(new Run(format[..position]));
            bool isTerms = position == terms;
            Uri uri = isTerms
                ? (region ?? NteOverseasRegion.Global).TermsUri
                : (region ?? NteOverseasRegion.Global).PrivacyUri;
            Button link = new()
            {
                Content = LoginText.Get(isTerms ? "TermsName" : "PrivacyName"),
                Theme = (ControlTheme)Application.Current!.FindResource("QuietButtonStyle")!,
                Padding = new(0), FontSize = 12, MinHeight = 0, VerticalAlignment = VerticalAlignment.Center
            };
            link.SetResourceReference(Button.ForegroundProperty, "AccentBrush");
            AutomationProperties.SetAutomationId(link, isTerms ? "terms" : "privacy");
            link.Click += (_, _) => Process.Start(new ProcessStartInfo(uri.AbsoluteUri) { UseShellExecute = true });
            text.Inlines!.Add(new InlineUIContainer { Child = link });
            format = format[(position + 3)..];
        }

        text.PointerReleased += (_, args) =>
        {
            if (args.Source is TextBlock) checkbox.IsChecked = checkbox.IsChecked != true;
        };
        Grid.SetColumn(text, 1);
        Children.Add(text);
    }
}
