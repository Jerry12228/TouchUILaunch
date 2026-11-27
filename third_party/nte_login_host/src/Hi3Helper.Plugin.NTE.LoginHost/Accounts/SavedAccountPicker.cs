using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;
using Avalonia.Automation;
using Avalonia.Controls.Templates;
using Nte.LoginHost.Accounts;
using Nte.LoginHost.Localization;

namespace Nte.LoginHost;

internal sealed record SavedAccountChoice(LocalAccount Account, string Primary, string Secondary, string Source)
{
    public string AccessibilityLabel =>
        string.Join(" · ", new[] { Primary, Secondary, Source }.Where(text => text.Length > 0));

    public override string ToString() => AccessibilityLabel;
}

internal sealed class SavedAccountPicker : StackPanel
{
    private readonly ListBox selector = new() { MaxHeight = 244, SelectionMode = SelectionMode.Single };
    private readonly TextBox remark = new() { MinHeight = 40, MaxLength = 32 };
    private readonly StackPanel editor = new(), confirmation = new();

    private readonly TextBlock confirmationContext = new()
    {
        FontSize = 13, FontWeight = FontWeight.SemiBold, TextWrapping = TextWrapping.Wrap, Margin = new(0, 0, 0, 12)
    };

    private readonly TextBlock context = new()
        { FontSize = 13, TextWrapping = TextWrapping.Wrap, Margin = new(0, 0, 0, 14) };

    private readonly TextBlock heading = new() { FontSize = 12, VerticalAlignment = VerticalAlignment.Center };
    private readonly Button manage;
    private bool reloading;
    private Func<LocalAccount, SavedAccountChoice> present;
    private IReadOnlyList<LocalAccount> accounts = [];
    internal bool IsManaging { get; private set; }
    internal bool HasAccounts => accounts.Count > 0;
    internal LocalAccount? Selected => (selector.SelectedItem as SavedAccountChoice)?.Account;
    internal string Remark => (remark.Text ?? "").Trim();
    internal event Action? SelectionChanged, LayoutChanged;
    internal event Func<Task>? RenameRequested, RemoveRequested;

    internal SavedAccountPicker(Func<string, ControlTheme> style, Func<LocalAccount, SavedAccountChoice> present)
    {
        this.present = present;
        selector.Theme = style("SavedAccountListStyle");
        selector.ItemTemplate = new FuncDataTemplate<SavedAccountChoice>((choice, _) =>
        {
            if (choice == null) return new Panel(); // Containers clear their content while recycling.
            StackPanel body = new();
            AutomationProperties.SetName(body, choice.AccessibilityLabel);
            ToolTip.SetTip(body, choice.AccessibilityLabel);
            Grid row = new() { ColumnDefinitions = new("*,Auto") };
            row.Children.Add(new TextBlock
            {
                Text = choice.Primary, FontSize = 14, FontWeight = FontWeight.SemiBold,
                TextWrapping = TextWrapping.Wrap, Margin = new(0, 0, 10, 0)
            });
            TextBlock source = new()
                { Text = choice.Source, FontSize = 10, Opacity = .7, VerticalAlignment = VerticalAlignment.Center };
            Grid.SetColumn(source, 1);
            row.Children.Add(source);
            body.Children.Add(row);
            body.Children.Add(new TextBlock
            {
                Text = choice.Secondary, FontSize = 12, Opacity = .78, Margin = new(0, 5, 0, 0),
                TextWrapping = TextWrapping.Wrap, MinHeight = 16, MaxHeight = 40,
                FlowDirection = FlowDirection.LeftToRight
            });
            return body;
        });
        AutomationProperties.SetAutomationId(selector, "account-selector");
        AutomationProperties.SetName(selector, LoginText.Get("SavedAccounts"));
        remark.Theme = style("LoginTextBoxStyle");
        AutomationProperties.SetAutomationId(remark, "remark");
        AutomationProperties.SetName(remark, LoginText.Get("AccountLabel"));
        Grid header = new() { Margin = new(0, 0, 0, 8) };
        header.ColumnDefinitions.Add(new());
        header.ColumnDefinitions.Add(new() { Width = GridLength.Auto });
        header.Children.Add(heading);
        manage = Button("ManageAccounts", "manage-accounts", "QuietButtonStyle");
        Grid.SetColumn(manage, 1);
        header.Children.Add(manage);
        Children.Add(header);
        Children.Add(selector);
        editor.Children.Add(context);
        editor.Children.Add(new TextBlock
            { Text = LoginText.Get("AccountLabel"), FontSize = 12, Margin = new(0, 0, 0, 7) });
        editor.Children.Add(remark);
        editor.Children.Add(Pair(Button("SaveLabel", "save-remark", "AccentButtonStyle"),
            Button("CancelEdit", "cancel-edit", "SmsButtonStyle")));
        Button remove = Button("GlobalRemoveAccount", "remove-account", "QuietButtonStyle");
        remove.Margin = new(0, 8, 0, 0);
        editor.Children.Add(remove);
        Children.Add(editor);
        confirmation.Children.Add(confirmationContext);
        confirmation.Children.Add(new TextBlock
        {
            Text = LoginText.Get("RemoveAccountConfirmation"), FontSize = 12,
            TextWrapping = TextWrapping.Wrap, Margin = new(0, 0, 0, 14)
        });
        confirmation.Children.Add(Pair(Button("ConfirmRemove", "confirm-remove", "AccentButtonStyle"),
            Button("CancelEdit", "cancel-remove", "SmsButtonStyle")));
        Children.Add(confirmation);
        selector.SelectionChanged += (_, _) =>
        {
            if (!reloading)
            {
                if (selector.SelectedItem != null) selector.ScrollIntoView(selector.SelectedItem);
                SelectionChanged?.Invoke();
            }
        };
        manage.Click += (_, _) => BeginManage();
        remove.Click += (_, _) =>
        {
            if (Selected == null) return;
            confirmationContext.Text = present(Selected).AccessibilityLabel;
            editor.IsVisible = false;
            confirmation.IsVisible = true;
            LayoutChanged?.Invoke();
        };
        foreach (Button button in editor.Children.OfType<Grid>().SelectMany(grid => grid.Children.OfType<Button>()))
            if (AutomationProperties.GetAutomationId(button) == "save-remark")
                button.Click += async (_, _) =>
                {
                    if (RenameRequested != null) await RenameRequested();
                };
            else button.Click += (_, _) => EndManage();
        foreach (Button button in confirmation.Children.OfType<Grid>()
                     .SelectMany(grid => grid.Children.OfType<Button>()))
            if (AutomationProperties.GetAutomationId(button) == "confirm-remove")
                button.Click += async (_, _) =>
                {
                    if (RemoveRequested != null) await RemoveRequested();
                };
            else
                button.Click += (_, _) =>
                {
                    confirmation.IsVisible = false;
                    editor.IsVisible = true;
                    LayoutChanged?.Invoke();
                };
        remark.KeyDown += (_, args) =>
        {
            if (args.Key == Avalonia.Input.Key.Escape)
            {
                EndManage();
                args.Handled = true;
            }
            else if (args.Key == Avalonia.Input.Key.Enter && editor.IsVisible)
            {
                Button save = editor.Children.OfType<Grid>().SelectMany(row => row.Children.OfType<Button>())
                    .Single(button => AutomationProperties.GetAutomationId(button) == "save-remark");
                save.RaiseEvent(new RoutedEventArgs(Avalonia.Controls.Button.ClickEvent));
                args.Handled = true;
            }
        };
        EndManage();

        Button Button(string key, string id, string styleName)
        {
            Button result = new()
            {
                Content = new TextBlock
                {
                    Text = LoginText.Get(key), TextWrapping = TextWrapping.Wrap, TextAlignment = TextAlignment.Center
                },
                Theme = style(styleName), MinHeight = 30, FontSize = 12, Padding = new(8, 6, 8, 6)
            };
            AutomationProperties.SetAutomationId(result, id);
            AutomationProperties.SetName(result, LoginText.Get(key));
            return result;
        }
    }

    private static Grid Pair(Button first, Button second)
    {
        Grid row = new() { Margin = new(0, 12, 0, 0) };
        row.ColumnDefinitions.Add(new());
        row.ColumnDefinitions.Add(new());
        first.Margin = new(0, 0, 5, 0);
        second.Margin = new(5, 0, 0, 0);
        row.Children.Add(first);
        Grid.SetColumn(second, 1);
        row.Children.Add(second);
        return row;
    }

    internal void Reload(IReadOnlyList<LocalAccount> values, Guid? selected = null,
        Func<LocalAccount, SavedAccountChoice>? presentation = null)
    {
        accounts = values;
        if (presentation != null) present = presentation;
        Guid? wanted = selected ?? Selected?.Id;
        reloading = true;
        selector.ItemsSource = values.Select(present).ToArray();
        selector.SelectedItem =
            selector.Items.Cast<SavedAccountChoice>().FirstOrDefault(item => item.Account.Id == wanted)
            ?? selector.Items.Cast<SavedAccountChoice>().FirstOrDefault();
        reloading = false;
        manage.IsEnabled = values.Count > 0;
        EndManage();
        SelectionChanged?.Invoke();
    }

    private void BeginManage()
    {
        if (Selected is not { } selected) return;
        IsManaging = true;
        heading.Text = LoginText.Get("ManageAccounts");
        context.Text = present(selected).AccessibilityLabel;
        remark.Text = selected.Label;
        selector.IsVisible = manage.IsVisible = false;
        editor.IsVisible = true;
        confirmation.IsVisible = false;
        LayoutChanged?.Invoke();
        remark.Focus();
        remark.SelectAll();
    }

    internal void EndManage()
    {
        IsManaging = false;
        heading.Text = LoginText.Get("SavedAccounts");
        editor.IsVisible = confirmation.IsVisible = false;
        selector.IsVisible = manage.IsVisible = true;
        LayoutChanged?.Invoke();
    }

    internal void RenamePreview(string label) =>
        Reload(
            accounts.Select(account => account.Id == Selected?.Id ? account with { Label = label } : account).ToArray(),
            Selected?.Id);

    internal void RemovePreview() => Reload(accounts.Where(account => account.Id != Selected?.Id).ToArray());
    internal IReadOnlyList<LocalAccount> Entries => accounts;

    internal string Reference(Guid id)
    {
        string text = id.ToString("N");
        int length = 8;
        while (length < 32 && accounts.Any(account =>
                   account.Id != id && account.Id.ToString("N").StartsWith(text[..length], StringComparison.Ordinal)))
            length += 4;
        return text[..length];
    }
}
