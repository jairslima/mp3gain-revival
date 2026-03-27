using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Platform.Storage;
using System.Collections.ObjectModel;

namespace MP3GainUI;

public partial class MainWindow : Window
{
    public ObservableCollection<MP3FileItem> FilesList { get; set; } = new();
    private MP3GainEngine _engine = new();

    public MainWindow()
    {
        InitializeComponent();
        FilesGrid.ItemsSource = FilesList;
    }

    private async void BtnAddFiles_Click(object? sender, RoutedEventArgs e)
    {
        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel == null) return;

        var files = await topLevel.StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title = "Selecionar Arquivos MP3",
            AllowMultiple = true,
            FileTypeFilter = new[] { new FilePickerFileType("Audio MP3") { Patterns = new[] { "*.mp3" } } }
        });

        foreach (var file in files)
        {
            FilesList.Add(new MP3FileItem { Path = file.Path.LocalPath });
        }
    }

    private void BtnClearAll_Click(object? sender, RoutedEventArgs e)
    {
        FilesList.Clear();
    }

    private async void BtnAnalyze_Click(object? sender, RoutedEventArgs e)
    {
        if (FilesList.Count == 0) return;

        // O baseline atual acompanha a referência ReplayGain do core.
        double targetVol = MP3GainEngine.ReplayGainReferenceLoudness;
        if (double.TryParse(TxtTargetVolume.Text?.Replace(",", "."), System.Globalization.NumberStyles.Any, System.Globalization.CultureInfo.InvariantCulture, out double parsed))
        {
            targetVol = parsed;
        }

        ProgressTotal.Maximum = FilesList.Count;
        ProgressTotal.Value = 0;

        foreach (var item in FilesList)
        {
            await _engine.AnalyzeFileAsync(item, targetVol);
            ProgressTotal.Value++;
        }
    }
}
