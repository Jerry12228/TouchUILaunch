using Nte.LoginHost.ViewModels;
using System.IO;
using Avalonia;
using Avalonia.Controls;
using Avalonia.Layout;
using Avalonia.Styling;
using Avalonia.Automation;

using Avalonia.Media;
using Avalonia.Media.Imaging;
using Nte.LoginHost.Accounts;
using Nte.LoginHost.Localization;

namespace Nte.LoginHost;

internal sealed partial class OverseasLoginView
{
    private void BuildLayout(Func<string, ControlTheme> style)
    {
        Configure(back, "BackToSavedAccounts", "back");
        back.HorizontalAlignment = HorizontalAlignment.Left;
        back.Margin = new(0, 0, 0, 10);
        Children.Add(back);
        savedBody.Children.Add(picker);
        Children.Add(savedBody);
        Configure(google, "SignInGoogle", "provider-google", "ProviderButtonStyle");
        Configure(facebook, "SignInFacebook", "provider-facebook", "ProviderButtonStyle");
        google.Content = ProviderLabel("google-g.png", "SignInGoogle");
        facebook.Content = ProviderLabel("facebook-primary.png", "SignInFacebook");
        Configure(emailEntry, "SignInEmail", "email-login", "SmsButtonStyle");
        Configure(phoneEntry, "PhoneSignIn", "phone-login", "SmsButtonStyle");
        phoneEntry.Margin = new(0, 0, 0, 10);
        methodsBody.Children.Add(phoneEntry);
        emailEntry.FontSize = 14;
        google.Margin = facebook.Margin = new(0, 0, 0, 10);
        methodsBody.Children.Add(google);
        methodsBody.Children.Add(facebook);
        methodsBody.Children.Add(emailEntry);
        newBody.Children.Add(methodsBody);
        email.Theme = emailCode.Theme = style("LoginTextBoxStyle");
        secret.Theme = style("LoginPasswordBoxStyle");
        AutomationProperties.SetName(email, LoginText.Get("Email"));
        AutomationProperties.SetAutomationId(email, "email");
        AutomationProperties.SetName(secret, LoginText.Get("Password"));
        AutomationProperties.SetAutomationId(secret, "secret");
        AutomationProperties.SetName(emailCode, LoginText.Get("EmailCode"));
        AutomationProperties.SetAutomationId(emailCode, "email-code");
        passwordMode.Content = LoginText.Get("PasswordSignIn");
        codeMode.Content = LoginText.Get("CodeSignIn");
        passwordMode.Theme = codeMode.Theme = style("LoginModeRadioStyle");
        AutomationProperties.SetAutomationId(passwordMode, "email-password-mode");
        AutomationProperties.SetAutomationId(codeMode, "email-code-mode");
        SlidingModeSelector rail = new(passwordMode, codeMode) { Margin = new(0, 0, 0, 14) };
        emailBody.Children.Add(rail);
        AutomationProperties.SetAutomationId(phoneCountry, "phone-country");
        phoneCountry.Theme = style("LoginComboBoxStyle");
        AutomationProperties.SetName(phoneCountry, LoginText.Get("CountryCode"));
        identifierRow.ColumnDefinitions.Add(new() { Width = GridLength.Auto });
        identifierRow.ColumnDefinitions.Add(new());
        phoneCountry.Width = 120; phoneCountry.Margin = new(0, 0, 8, 0);
        identifierRow.Children.Add(phoneCountry);
        Grid.SetColumn(email, 1); identifierRow.Children.Add(email);
        emailBody.Children.Add(LabeledGroup(identifierLabel, identifierRow));
        passwordGroup.Children.Add(Group("Password", secret));
        emailBody.Children.Add(passwordGroup);
        Grid codeRow = new();
        codeRow.ColumnDefinitions.Add(new());
        codeRow.ColumnDefinitions.Add(new() { Width = GridLength.Auto });
        codeRow.Children.Add(emailCode);
        Configure(captcha, "SendEmailCode", "send-code", "SmsButtonStyle");
        captcha.MinHeight = 42;
        captcha.MinWidth = 86;
        captcha.MaxWidth = 150;
        captcha.Margin = new(8, 0, 0, 0);
        Grid.SetColumn(captcha, 1);
        codeRow.Children.Add(captcha);
        codeGroup.Children.Add(LabeledGroup(codeLabel, codeRow));
        emailBody.Children.Add(codeGroup);
        newBody.Children.Add(emailBody);
        remember.Theme = style("FormCheckBoxStyle");
        remember.Content = new TextBlock { Text = LoginText.Get("SaveOnDevice"), TextWrapping = TextWrapping.Wrap };
        remember.Margin = new(0, 12, 0, 0);
        AutomationProperties.SetAutomationId(remember, "remember");
        AutomationProperties.SetName(remember, LoginText.Get("SaveOnDevice"));
        newBody.Children.Add(remember);
        newBody.Children.Add(new GlobalAgreementBlock(agreement, style("FormCheckBoxStyle"), model.Region));
        Children.Add(newBody);
        status.Margin = new(0, 10, 0, 4);
        AutomationProperties.SetAutomationId(status, "status");
        AutomationProperties.SetLiveSetting(status, Avalonia.Automation.AutomationLiveSetting.Polite);
        Children.Add(status);
        Configure(signIn, "ContinueSignIn", "submit", "AccentButtonStyle");
        signIn.Margin = new(0, 12, 0, 0);
        Configure(otherAccount, "UseOtherAccount", "other-account");
        otherAccount.Margin = new(0, 6, 0, 0);
        Configure(retry, "RetryConnection", "retry-connection", "SmsButtonStyle");
        Configure(cancelWork, "CancelOperation", "cancel-operation", "SmsButtonStyle");
        cancelWork.Margin = new(0, 14, 0, 0);
        Configure(help, "SignInHelp", "help");
        help.Margin = new(0, 12, 0, 0);
        help.FontSize = 11;
        Children.Add(signIn);
        Children.Add(otherAccount);
        Children.Add(retry);
        Children.Add(cancelWork);

        void Configure(Button button, string key, string id, string styleName = "QuietButtonStyle")
        {
            button.Theme = style(styleName);
            button.Content = new TextBlock
                { Text = LoginText.Get(key), TextWrapping = TextWrapping.Wrap, TextAlignment = TextAlignment.Center };
            button.MinHeight = styleName == "QuietButtonStyle" ? 28 : 44;
            button.Padding = new(12, 8, 12, 8);
            button.FontSize = styleName == "AccentButtonStyle" ? 14 : 12;
            AutomationProperties.SetAutomationId(button, id);
            AutomationProperties.SetName(button, LoginText.Get(key));
        }
    }

    private static StackPanel Group(string key, Control input)
    {
        StackPanel group = new() { Margin = new(0, 0, 0, 12) };
        group.Children.Add(new TextBlock { Text = LoginText.Get(key), FontSize = 12, Margin = new(0, 0, 0, 7) });
        group.Children.Add(input);
        return group;
    }

    private static StackPanel LabeledGroup(TextBlock label, Control input)
    {
        StackPanel group = new() { Margin = new(0, 0, 0, 12) };
        label.FontSize = 12; label.Margin = new(0, 0, 0, 7);
        group.Children.Add(label); group.Children.Add(input); return group;
    }

    private static Grid ProviderLabel(string asset, string key)
    {
        Grid row = new();
        row.ColumnDefinitions.Add(new() { Width = new(30) });
        row.ColumnDefinitions.Add(new());
        row.ColumnDefinitions.Add(new() { Width = new(30) });
        Bitmap bitmap = new(Path.Combine(AppContext.BaseDirectory, "Assets", "Brand", asset));
        Image image = new()
        {
            Source = bitmap, Width = 20, Height = 20, Stretch = Stretch.Uniform,
            HorizontalAlignment = HorizontalAlignment.Left
        };
        RenderOptions.SetBitmapInterpolationMode(image, BitmapInterpolationMode.HighQuality);
        row.Children.Add(image);
        TextBlock text = new()
        {
            Text = LoginText.Get(key), FontSize = 14, FontWeight = FontWeight.Medium,
            TextWrapping = TextWrapping.Wrap, TextAlignment = TextAlignment.Center,
            VerticalAlignment = VerticalAlignment.Center
        };
        Grid.SetColumn(text, 1);
        row.Children.Add(text);
        return row;
    }

    private void RefreshLayout()
    {
        updatingFields = true;
        try
        {
            if (email.Text != model.Identifier) email.Text = model.Identifier;
            if (emailCode.Text != model.EmailCode) emailCode.Text = model.EmailCode;
            if (!ReferenceEquals(phoneCountry.ItemsSource, model.PhoneCountries)) phoneCountry.ItemsSource = model.PhoneCountries;
            phoneCountry.SelectedItem = model.PhoneCountries.FirstOrDefault(country => country.CountryCode == model.CountryCode);
            remember.IsChecked = model.Remember;
            agreement.IsChecked = model.Agreement;
            status.Text = model.Status;
            passwordMode.IsChecked = model.UsePassword;
            codeMode.IsChecked = !model.UsePassword;
        }
        finally { updatingFields = false; }
        bool busy = model.IsBusy;
        var page = model.Page;
        bool phone = page == GlobalLoginPage.Phone;
        bool credentials = phone || page == GlobalLoginPage.Email;
        savedBody.IsVisible = !busy && page == GlobalLoginPage.Saved;
        newBody.IsVisible = !busy && (page == GlobalLoginPage.Methods || credentials);
        methodsBody.IsVisible = page == GlobalLoginPage.Methods;
        emailEntry.IsVisible = model.SupportsEmail;
        phoneEntry.IsVisible = model.SupportsPhone;
        emailBody.IsVisible = credentials;
        phoneCountry.IsVisible = phone;
        identifierLabel.Text = LoginText.Get(phone ? "Phone" : "Email");
        codeLabel.Text = LoginText.Get(phone ? "SmsCode" : "EmailCode");
        AutomationProperties.SetName(email, identifierLabel.Text);
        AutomationProperties.SetAutomationId(email, phone ? "phone" : "email");
        AutomationProperties.SetName(emailCode, codeLabel.Text);
        AutomationProperties.SetAutomationId(emailCode, phone ? "phone-code" : "email-code");
        AutomationProperties.SetAutomationId(passwordMode, phone ? "phone-password-mode" : "email-password-mode");
        AutomationProperties.SetAutomationId(codeMode, phone ? "phone-code-mode" : "email-code-mode");
        helpBody.IsVisible = !busy && page == GlobalLoginPage.Help;
        back.IsVisible = !busy && (credentials || page == GlobalLoginPage.Help || page == GlobalLoginPage.Methods && picker.HasAccounts);
        back.Content = LoginText.Get(credentials ? "BackToMethods" : page == GlobalLoginPage.Help ? "BackToSignIn" : "BackToSavedAccounts");
        signIn.IsVisible = !busy && (credentials || page == GlobalLoginPage.Saved && !picker.IsManaging);
        otherAccount.IsVisible = !busy && page == GlobalLoginPage.Saved && !picker.IsManaging;
        retry.IsVisible = !busy && !model.IsPreview && !model.Ready && page == GlobalLoginPage.Methods;
        cancelWork.IsVisible = busy;
        help.IsVisible = false;
        status.IsVisible = !(string.IsNullOrWhiteSpace(model.Status));
        passwordGroup.IsVisible = model.UsePassword;
        codeGroup.IsVisible = !(model.UsePassword);
        email.IsEnabled = secret.IsEnabled = emailCode.IsEnabled = passwordMode.IsEnabled = codeMode.IsEnabled = !busy;
        phoneCountry.IsEnabled = !busy;
        agreement.IsEnabled = remember.IsEnabled = picker.IsEnabled = !busy;
        captcha.Content = model.CaptchaLabel; signIn.Content = model.SubmitLabel;
        signIn.IsDefault = model.SubmitCommand.CanExecute(null) && !picker.IsManaging;
        LayoutChanged?.Invoke();
    }

    private SavedAccountChoice Present(LocalAccount account)
    {
        model.Profiles.TryGetValue(account.Id, out AccountDisplayProfile? profile);
        bool custom = profile?.InitialLabel == null || account.Label != profile.InitialLabel;
        string primary = custom ? account.Label : profile?.MaskedPhone ?? profile?.Email ?? profile?.Name ?? account.Label;
        string secondary = profile?.Email != null && primary != profile.Email ? profile.Email
            : profile?.Email != null ? profile.Name ?? "" : profile?.Provider == 7 ? LoginText.Get("Phone") : LoginText.Get("EmailNotProvided") + " · " + LoginText.Get("SavedAccountNumber", picker.Reference(account.Id));
        string source = profile?.Provider switch { 2 => "Google", 1 => "Facebook", 7 => LoginText.Get("Phone"), 8 => LoginText.Get("Email"), _ => LoginText.Get("LegacyAccount") };
        return new(account, primary, secondary, source);
    }
    internal void SetPreviewProfileScenario(string variant) => model.SetPreviewProfileScenario(variant);
    internal void SetPreviewAccounts(int count) => model.SetPreviewAccounts(count);
}
