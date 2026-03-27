using Avalonia.Controls;
using Avalonia.Interactivity;
using Avalonia.Platform.Storage;
using System;
using System.Collections.ObjectModel;
using System.Globalization;
using System.IO;
using System.Linq;
using System.Diagnostics;
using System.Threading;
using System.Threading.Tasks;

namespace MP3GainUI;

public partial class MainWindow : Window
{
    public ObservableCollection<MP3FileItem> FilesList { get; set; } = new();
    private readonly MP3GainEngine _engine = new();
    private CancellationTokenSource? _operationCts;

    public MainWindow()
    {
        InitializeComponent();
        FilesGrid.ItemsSource = FilesList;
        TxtTargetVolume.Text = MP3GainEngine.ReplayGainReferenceLoudness.ToString("0.0", CultureInfo.InvariantCulture).Replace(".", ",");
        SetStatus("Pronto. Adicione arquivos ou uma pasta para começar.");
    }

    private async void BtnAddFiles_Click(object? sender, RoutedEventArgs e)
    {
        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel == null)
        {
            SetStatus("Erro: a janela não conseguiu obter o provedor de arquivos.");
            return;
        }

        if (!topLevel.StorageProvider.CanOpen)
        {
            SetStatus("Erro: o diálogo de arquivos não está disponível neste ambiente.");
            return;
        }

        var files = await topLevel.StorageProvider.OpenFilePickerAsync(new FilePickerOpenOptions
        {
            Title = "Selecionar Arquivos MP3",
            AllowMultiple = true,
            FileTypeFilter = new[] { new FilePickerFileType("Audio MP3") { Patterns = new[] { "*.mp3" } } }
        });

        foreach (var file in files)
        {
            AddFileIfMissing(file.Path.LocalPath);
        }

        SetStatus(files.Count == 0 ? "Nenhum arquivo foi selecionado." : $"{files.Count} arquivo(s) adicionados.");
    }

    private async void BtnAddFolder_Click(object? sender, RoutedEventArgs e)
    {
        var topLevel = TopLevel.GetTopLevel(this);
        if (topLevel == null)
        {
            SetStatus("Erro: a janela não conseguiu obter o provedor de pastas.");
            return;
        }

        if (!topLevel.StorageProvider.CanPickFolder)
        {
            SetStatus("Erro: o diálogo de pastas não está disponível neste ambiente.");
            return;
        }

        var folders = await topLevel.StorageProvider.OpenFolderPickerAsync(new FolderPickerOpenOptions
        {
            Title = "Selecionar Pasta com MP3",
            AllowMultiple = false
        });

        string? folder = folders.FirstOrDefault()?.Path.LocalPath;
        if (string.IsNullOrWhiteSpace(folder) || !Directory.Exists(folder))
        {
            SetStatus("Nenhuma pasta foi selecionada.");
            return;
        }

        int added = 0;
        foreach (string path in Directory.EnumerateFiles(folder, "*.mp3", SearchOption.AllDirectories))
        {
            int before = FilesList.Count;
            AddFileIfMissing(path);
            if (FilesList.Count > before) added++;
        }

        SetStatus(added == 0 ? "Nenhum arquivo MP3 novo foi encontrado na pasta." : $"{added} arquivo(s) adicionados da pasta.");
    }

    private void BtnClearFiles_Click(object? sender, RoutedEventArgs e)
    {
        var selected = FilesGrid.SelectedItems?.OfType<MP3FileItem>().ToList();
        if (selected == null || selected.Count == 0)
        {
            SetStatus("Selecione um ou mais arquivos para remover.");
            return;
        }

        foreach (var item in selected)
        {
            FilesList.Remove(item);
        }

        SetStatus($"{selected.Count} arquivo(s) removidos da lista.");
    }

    private void BtnClearAll_Click(object? sender, RoutedEventArgs e)
    {
        FilesList.Clear();
        SetStatus("Lista limpa.");
    }

    private async void BtnAnalyze_Click(object? sender, RoutedEventArgs e)
    {
        if (FilesList.Count == 0) return;
        await RunBatchAsync(async (item, targetVol, cancellationToken) =>
        {
            cancellationToken.ThrowIfCancellationRequested();
            await _engine.AnalyzeFileAsync(item, targetVol);
        });
    }

    private async void BtnGain_Click(object? sender, RoutedEventArgs e)
    {
        if (FilesList.Count == 0) return;
        await RunBatchAsync(async (item, targetVol, cancellationToken) =>
        {
            await _engine.ApplyTrackGainAsync(item.Path, targetVol, cancellationToken);
            await _engine.AnalyzeFileAsync(item, targetVol);
        });
    }

    private void BtnCancel_Click(object? sender, RoutedEventArgs e)
    {
        _operationCts?.Cancel();
        SetStatus("Cancelamento solicitado.");
    }

    private void BtnPlaySelected_Click(object? sender, RoutedEventArgs e)
    {
        MP3FileItem? selected = FilesGrid.SelectedItems?.OfType<MP3FileItem>().FirstOrDefault();
        if (selected == null)
        {
            SetStatus("Selecione um arquivo para reproduzir.");
            return;
        }

        if (!File.Exists(selected.Path))
        {
            SetStatus("O arquivo selecionado não foi encontrado no disco.");
            return;
        }

        try
        {
            Process.Start(new ProcessStartInfo
            {
                FileName = selected.Path,
                UseShellExecute = true
            });
            SetStatus($"Reprodução aberta no player padrão: {Path.GetFileName(selected.Path)}");
        }
        catch (Exception ex)
        {
            SetStatus($"Erro ao abrir o player padrão: {ex.Message}");
        }
    }

    private async void MenuAbout_Click(object? sender, RoutedEventArgs e)
    {
        var aboutWindow = new AboutWindow(_engine.GetResolvedCliPathOrNull());
        await aboutWindow.ShowDialog(this);
        SetStatus("Janela Sobre exibida.");
    }

    private void MenuExit_Click(object? sender, RoutedEventArgs e)
    {
        Close();
    }

    private async Task RunBatchAsync(Func<MP3FileItem, double, CancellationToken, Task> operation)
    {
        double targetVol = GetTargetVolume();
        _operationCts?.Dispose();
        _operationCts = new CancellationTokenSource();

        try
        {
            SetBusy(true);
            SetStatus("Processando...");
            ProgressFile.IsIndeterminate = false;
            ProgressFile.Maximum = 1;
            ProgressFile.Value = 0;
            ProgressTotal.IsIndeterminate = false;
            ProgressTotal.Maximum = FilesList.Count;
            ProgressTotal.Value = 0;

            foreach (var item in FilesList.ToList())
            {
                _operationCts.Token.ThrowIfCancellationRequested();
                ProgressFile.Value = 0;
                await operation(item, targetVol, _operationCts.Token);
                ProgressFile.Value = 1;
                ProgressTotal.Value++;
                SetStatus($"Processado: {System.IO.Path.GetFileName(item.Path)}");
            }

            SetStatus("Operação concluída.");
        }
        catch (OperationCanceledException)
        {
            SetStatus("Operação cancelada.");
        }
        catch (Exception ex)
        {
            SetStatus($"Erro: {ex.Message}");
        }
        finally
        {
            SetBusy(false);
            _operationCts.Dispose();
            _operationCts = null;
        }
    }

    private double GetTargetVolume()
    {
        if (double.TryParse(TxtTargetVolume.Text?.Replace(",", "."), NumberStyles.Any, CultureInfo.InvariantCulture, out double parsed))
        {
            return parsed;
        }

        return MP3GainEngine.ReplayGainReferenceLoudness;
    }

    private void SetBusy(bool isBusy)
    {
        BtnAddFiles.IsEnabled = !isBusy;
        BtnAddFolder.IsEnabled = !isBusy;
        BtnAnalyze.IsEnabled = !isBusy;
        BtnGain.IsEnabled = !isBusy;
        BtnPlaySelected.IsEnabled = !isBusy;
        BtnClearFiles.IsEnabled = !isBusy;
        BtnClearAll.IsEnabled = !isBusy;
        BtnExit.IsEnabled = !isBusy;
        BtnCancel.IsEnabled = isBusy;
    }

    private void SetStatus(string message)
    {
        TxtStatus.Text = message;
        Title = $"MP3Gain Revival - {message}";
    }

    private void AddFileIfMissing(string path)
    {
        if (FilesList.Any(item => string.Equals(item.Path, path, StringComparison.OrdinalIgnoreCase)))
        {
            return;
        }

        FilesList.Add(new MP3FileItem { Path = path });
    }
}
