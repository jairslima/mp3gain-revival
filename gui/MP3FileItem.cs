using System.ComponentModel;
using System.Runtime.CompilerServices;
using System.Collections.Generic;

namespace MP3GainUI;

public class MP3FileItem : INotifyPropertyChanged
{
    private string _path = string.Empty;
    private string _volume = string.Empty;
    private string _clipping = string.Empty;
    private string _trackGain = string.Empty;
    private string _trackClip = string.Empty;
    private string _albumVolume = string.Empty;
    private string _albumGain = string.Empty;
    private string _albumClip = string.Empty;

    public string Path { get => _path; set => SetField(ref _path, value); }
    public string Volume { get => _volume; set => SetField(ref _volume, value); }
    public string Clipping { get => _clipping; set => SetField(ref _clipping, value); }
    public string TrackGain { get => _trackGain; set => SetField(ref _trackGain, value); }
    public string TrackClip { get => _trackClip; set => SetField(ref _trackClip, value); }
    public string AlbumVolume { get => _albumVolume; set => SetField(ref _albumVolume, value); }
    public string AlbumGain { get => _albumGain; set => SetField(ref _albumGain, value); }
    public string AlbumClip { get => _albumClip; set => SetField(ref _albumClip, value); }

    public event PropertyChangedEventHandler? PropertyChanged;

    protected void OnPropertyChanged([CallerMemberName] string? propertyName = null)
    {
        PropertyChanged?.Invoke(this, new PropertyChangedEventArgs(propertyName));
    }

    protected bool SetField<T>(ref T field, T value, [CallerMemberName] string? propertyName = null)
    {
        if (EqualityComparer<T>.Default.Equals(field, value)) return false;
        field = value;
        OnPropertyChanged(propertyName);
        return true;
    }
}
