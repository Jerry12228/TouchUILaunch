using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;
using Avalonia.Controls.Templates;
using Avalonia.Threading;
using Nte.Login;
using Avalonia.Input;
using Nte.LoginHost.Services;
using Nte.LoginHost.ViewModels;

namespace Nte.LoginHost;

internal sealed partial class OverseasLoginView : StackPanel, IDisposable
{
    private readonly OverseasLoginViewModel model;
    private readonly SavedAccountPicker picker;

    private readonly StackPanel savedBody = new(),
        newBody = new(),
        methodsBody = new(),
        emailBody = new(),
        helpBody = new();

    private readonly CheckBox remember = new() { IsChecked = true }, agreement = new();
    private readonly TextBox email = new() { Height = 42, MaxLength = 128 };

    private readonly TextBox secret = new()
        { Height = 42, MaxLength = 256, PasswordChar = '\u25CF', IsUndoEnabled = false };

    private readonly TextBox emailCode = new() { Height = 42, MaxLength = 16 };

    private readonly RadioButton passwordMode = new() { GroupName = "EmailMethod", IsChecked = true },
        codeMode = new() { GroupName = "EmailMethod" };

    private readonly Button google = new(), facebook = new(), emailEntry = new(), signIn = new(), captcha = new();
    private readonly Button phoneEntry = new();

    private readonly ComboBox phoneCountry = new()
        { MinHeight = 42, HorizontalContentAlignment = HorizontalAlignment.Stretch };

    private readonly TextBlock identifierLabel = new(), codeLabel = new();
    private readonly Grid identifierRow = new();
    private readonly Button otherAccount = new(), back = new(), help = new(), retry = new(), cancelWork = new();
    private readonly TextBlock status = new() { FontSize = 12, TextWrapping = TextWrapping.Wrap };
    private readonly StackPanel passwordGroup = new(), codeGroup = new();
    private readonly Avalonia.Threading.DispatcherTimer cooldown = new() { Interval = TimeSpan.FromSeconds(1) };
    internal string TitleLabel => model.Title;
    internal Nte.Login.NteHostRegion Region => model.Region.HostRegion;
    internal event Action? LayoutChanged;

    internal OverseasLoginView(IOverseasLoginService service, Func<string, ControlTheme> style, bool preview = false,
        string? notice = null)
    {
        model = new(service, new PasswordSecret(secret), key => Localization.LoginText.Get(key),
            OverseasLoginService.PresentError, preview, notice);
        DataContext = model;
        picker = new(style, Present);
        BuildLayout(style);
        // Typed event wiring keeps Native AOT free of reflection bindings.
        phoneCountry.ItemTemplate = new FuncDataTemplate<NtePhoneCountry>((country, _) =>
            new TextBlock { Text = country?.DisplayName });
        email.TextChanged += (_, _) =>
        {
            if (!updatingFields) model.Identifier = email.Text ?? "";
        };
        emailCode.TextChanged += (_, _) =>
        {
            if (!updatingFields) model.EmailCode = emailCode.Text ?? "";
        };
        phoneCountry.SelectionChanged += (_, _) =>
        {
            if (!updatingFields && phoneCountry.SelectedItem is NtePhoneCountry country)
                model.CountryCode = country.CountryCode;
        };
        remember.IsCheckedChanged += (_, _) =>
        {
            if (!updatingFields) model.Remember = remember.IsChecked == true;
        };
        agreement.IsCheckedChanged += (_, _) =>
        {
            if (!updatingFields) model.Agreement = agreement.IsChecked == true;
        };
        google.Command = model.GoogleCommand;
        facebook.Command = model.FacebookCommand;
        emailEntry.Command = model.EmailCommand;
        signIn.Command = model.SubmitCommand;
        captcha.Command = model.SendCodeCommand;
        phoneEntry.Command = model.PhoneCommand;
        otherAccount.Command = model.MethodsCommand;
        back.Command = model.BackCommand;
        help.Command = model.HelpCommand;
        retry.Command = model.RetryCommand;
        cancelWork.Command = model.CancelOperationCommand;
        picker.SelectionChanged += () => model.SelectedAccountId = picker.Selected?.Id;
        picker.LayoutChanged += RefreshLayout;
        picker.RenameRequested += async () =>
        {
            model.Remark = picker.Remark;
            await model.RenameCommand.ExecuteAsync();
        };
        picker.RemoveRequested += () => model.RemoveCommand.ExecuteAsync();
        passwordMode.IsCheckedChanged += (_, _) =>
        {
            if (!updatingFields && passwordMode.IsChecked == true) model.UsePassword = true;
        };
        codeMode.IsCheckedChanged += (_, _) =>
        {
            if (!updatingFields && codeMode.IsChecked == true) model.UsePassword = false;
        };
        model.PropertyChanged += (_, args) =>
        {
            if (args.PropertyName == nameof(model.Accounts)) picker.Reload(model.Accounts, model.SelectedAccountId);
            if (args.PropertyName == nameof(model.Page))
            {
                picker.EndManage();
                if (model.Page is GlobalLoginPage.Email or GlobalLoginPage.Phone)
                    Dispatcher.UIThread.Post(() => email.Focus());
            }

            RefreshLayout();
        };
        cooldown.Tick += (_, _) => model.Tick();
        cooldown.Start();
        KeyDown += async (_, args) =>
        {
            if (args.Key == Key.Escape && model.IsBusy)
            {
                model.CancelOperationCommand.Execute(null);
                args.Handled = true;
            }
            else if (args.Key == Key.Escape && picker.IsManaging)
            {
                picker.EndManage();
                args.Handled = true;
            }
            else if (args.Key == Key.Escape &&
                     model.Page is GlobalLoginPage.Email or GlobalLoginPage.Phone or GlobalLoginPage.Help)
            {
                model.BackCommand.Execute(null);
                args.Handled = true;
            }
            else if (args.Key == Key.Enter && !picker.IsManaging && model.SubmitCommand.CanExecute(null))
            {
                args.Handled = true;
                await model.SubmitCommand.ExecuteAsync();
            }
        };
        picker.Reload(model.Accounts, model.SelectedAccountId);
        if (preview) model.SetPreviewAccounts(3);
        RefreshLayout();
    }

    private bool updatingFields;
    internal Task<bool> RunAsync(CancellationToken token) => model.RunAsync(token);
    internal void SetPreviewBusy(bool busy) => model.SetPreviewBusy(busy);
    internal void ShowHelp() => model.ShowHelp();
    internal void ShowNotice(string text) => model.ShowNotice(text);
    internal void Cancel() => model.Cancel();

    public void Dispose()
    {
        cooldown.Stop();
        model.Dispose();
    }

    private sealed class PasswordSecret(TextBox input) : ITransientLoginSecret
    {
        public string Consume()
        {
            string value = input.Text ?? "";
            input.Clear();
            return value;
        }

        public void Clear() => input.Clear();
    }
}
