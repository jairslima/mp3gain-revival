using System.Diagnostics;
using System.IO;
using System.Threading.Tasks;
using System.Text.RegularExpressions;
using Avalonia.Threading;
using System;

namespace MP3GainUI;

public class MP3GainEngine
{
    public const double ReplayGainReferenceLoudness = 95.5;

    private string? TryGetCliPath()
    {
        string localPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "mp3gain.exe");
        if (File.Exists(localPath)) return localPath;
        
        // Caminho de fallback de desenvolvedor
        string devPath = Path.Combine(AppDomain.CurrentDomain.BaseDirectory, "..", "..", "..", "..", "..", "build", "Release", "mp3gain.exe");
        if (File.Exists(devPath)) return devPath;

        return null;
    }

    private string GetCliPath()
    {
        string? cliPath = TryGetCliPath();
        if (!string.IsNullOrEmpty(cliPath))
        {
            return cliPath;
        }

        throw new FileNotFoundException(
            "mp3gain.exe nao foi encontrado ao lado da GUI. Gere o CLI em build\\Release e reconstrua a GUI para formar o produto unificado."
        );
    }

    public async Task AnalyzeFileAsync(MP3FileItem item, double targetVolume)
    {
        // O CLI usa 95.5 dB como referência ReplayGain.
        // O modificador /d calcula a diferença para o TargetVolume do usuário
        double mod = targetVolume - ReplayGainReferenceLoudness;
        string modArg = mod != 0 ? $"/d {mod.ToString(System.Globalization.CultureInfo.InvariantCulture)}" : "";

        var tcs = new TaskCompletionSource();

        using var process = new Process();
        process.StartInfo.FileName = GetCliPath();
        process.StartInfo.Arguments = $"/o {modArg} \"{item.Path}\""; // /o gera saída no formato banco de dados tabulado
        process.StartInfo.UseShellExecute = false;
        process.StartInfo.RedirectStandardOutput = true;
        process.StartInfo.RedirectStandardError = true;
        process.StartInfo.CreateNoWindow = true;

        process.OutputDataReceived += (s, e) =>
        {
            if (!string.IsNullOrWhiteSpace(e.Data) && !e.Data.StartsWith("File"))
            {
                var cols = e.Data.Split('\t');
                if (cols.Length >= 6 && cols[0] != "\"Album\"")
                {
                    Dispatcher.UIThread.Post(() =>
                    {
                        // Exemplo tabulado: Arquivo | mp3 gain | db gain | max amp | max global gain | min global gain
                        // Calculando volume final com base na recomendação
                        double dbGain = double.Parse(cols[2], System.Globalization.CultureInfo.InvariantCulture);
                        double currentVol = targetVolume - dbGain;
                        
                        item.Volume = currentVol.ToString("0.0");
                        item.TrackGain = cols[1];
                        
                        double maxAmp = double.Parse(cols[3], System.Globalization.CultureInfo.InvariantCulture);
                        // No mp3gain C original, a amplitude limite ideal sem clip é por volta de 32767
                        if (maxAmp > 32767.0) {
                            item.Clipping = "S";
                        } else {
                            item.Clipping = "";
                        }
                    });
                }
            }
        };

        process.EnableRaisingEvents = true;
        process.Exited += (s, e) => tcs.TrySetResult();

        process.Start();
        process.BeginOutputReadLine();

        await tcs.Task;
    }
}
