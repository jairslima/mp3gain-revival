using Avalonia.Controls;
using Avalonia.Interactivity;
using System;
using System.IO;
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

        string version =
            Assembly.GetEntryAssembly()?.GetName().Version?.ToString()
            ?? Assembly.GetExecutingAssembly().GetName().Version?.ToString()
            ?? "desconhecida";

        TxtVersion.Text = version;
        TxtReference.Text = $"{MP3GainEngine.ReplayGainReferenceLoudness:0.0} dB";
        TxtCliPath.Text = string.IsNullOrWhiteSpace(cliPath) ? "não localizado" : cliPath;
        TxtGuiPath.Text = Environment.ProcessPath ?? Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "MP3GainUI.exe");
    }

    private void CloseButton_Click(object? sender, RoutedEventArgs e)
    {
        Close();
    }
}
