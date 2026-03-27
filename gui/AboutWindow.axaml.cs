using Avalonia.Controls;
using Avalonia.Interactivity;
using System.Reflection;

namespace MP3GainUI;

public partial class AboutWindow : Window
{
    public AboutWindow() : this(null)
    {
    }

    public AboutWindow(string? cliPath)
    {
        InitializeComponent();

        string frontendVersion =
            Assembly.GetEntryAssembly()?.GetName().Version?.ToString()
            ?? Assembly.GetExecutingAssembly().GetName().Version?.ToString()
            ?? "desconhecida";

        TxtFrontendVersion.Text = $"Versão {frontendVersion}";
        TxtBackendVersion.Text = "Versão 1.6.2";
        TxtReference.Text = $"Referência ReplayGain atual: {MP3GainEngine.ReplayGainReferenceLoudness:0.0} dB.";
    }

    private void CloseButton_Click(object? sender, RoutedEventArgs e)
    {
        Close();
    }
}
