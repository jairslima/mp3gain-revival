#ifndef MP3GAIN_CLI_H
#define MP3GAIN_CLI_H

struct MP3GainCliOptions {
    int ignoreClipWarning;
    int autoClip;
    int applyTrack;
    int applyAlbum;
    int analysisTrack;
    int fileStart;
    int databaseFormat;
    int directGain;
    int directSingleChannelGain;
    int directGainVal;
    int mp3GainMod;
    double dBGainMod;
};

void mp3gain_cli_apply_options(
    const struct MP3GainCliOptions *options,
    int *fileStart,
    int *ignoreClipWarning,
    int *autoClip,
    int *applyTrack,
    int *applyAlbum,
    int *analysisTrack,
    int *databaseFormat,
    int *directGain,
    int *directSingleChannelGain,
    int *directGainVal,
    int *mp3GainMod,
    double *dBGainMod
);
void mp3gain_cli_init(struct MP3GainCliOptions *options);
void mp3gain_cli_parse(int argc, char **argv, struct MP3GainCliOptions *options);
void mp3gain_cli_print_table_header(int databaseFormat, int checkTagOnly, int undoChanges);

#endif


