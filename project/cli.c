#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"

void errUsage(char *progname);
void fullUsage(char *progname);
void wrapExplanation(void);
void showVersion(char *argv0);

extern unsigned char maxAmpOnly;
extern short int saveTime;
extern int whichChannel;
extern int Reckless;
extern int wrapGain;
extern int undoChanges;
extern int skipTag;
extern int deleteTag;
extern int forceRecalculateTag;
extern int forceUpdateTag;
extern int checkTagOnly;
extern int QuietMode;
extern int UsingTemp;
extern int useId3;

static int is_single_char_option(char *arg)
{
#ifdef WIN32
    return (arg[0] == '/') || ((arg[0] == '-') && (strlen(arg) == 2));
#else
    return ((arg[0] == '/') || (arg[0] == '-')) && (strlen(arg) == 2);
#endif
}

void mp3gain_cli_init(struct MP3GainCliOptions *options)
{
    options->ignoreClipWarning = 0;
    options->autoClip = 0;
    options->applyTrack = 0;
    options->applyAlbum = 0;
    options->analysisTrack = 0;
    options->fileStart = 1;
    options->databaseFormat = 0;
    options->directGain = 0;
    options->directSingleChannelGain = 0;
    options->directGainVal = 0;
    options->mp3GainMod = 0;
    options->dBGainMod = 0;
}

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
)
{
    *fileStart = options->fileStart;
    *ignoreClipWarning = options->ignoreClipWarning;
    *autoClip = options->autoClip;
    *applyTrack = options->applyTrack;
    *applyAlbum = options->applyAlbum;
    *analysisTrack = options->analysisTrack;
    *databaseFormat = options->databaseFormat;
    *directGain = options->directGain;
    *directSingleChannelGain = options->directSingleChannelGain;
    *directGainVal = options->directGainVal;
    *mp3GainMod = options->mp3GainMod;
    *dBGainMod = options->dBGainMod;
}

void mp3gain_cli_parse(int argc, char **argv, struct MP3GainCliOptions *options)
{
    int i;
    char chtmp;

    if (argc < 2) {
        errUsage(argv[0]);
    }

    maxAmpOnly = 0;
    saveTime = 0;

    for (i = 1; i < argc; i++) {
        if (is_single_char_option(argv[i])) {
            options->fileStart++;
            switch (argv[i][1]) {
                case 'a':
                case 'A':
                    options->applyTrack = 0;
                    options->applyAlbum = !0;
                    break;
                case 'c':
                case 'C':
                    options->ignoreClipWarning = !0;
                    break;
                case 'd':
                case 'D':
                    if (argv[i][2] != '\0') {
                        options->dBGainMod = atof(argv[i] + 2);
                    } else if (i + 1 < argc) {
                        options->dBGainMod = atof(argv[i + 1]);
                        i++;
                        options->fileStart++;
                    } else {
                        errUsage(argv[0]);
                    }
                    break;
                case 'f':
                case 'F':
                    Reckless = 1;
                    break;
                case 'g':
                case 'G':
                    options->directGain = !0;
                    options->directSingleChannelGain = 0;
                    if (argv[i][2] != '\0') {
                        options->directGainVal = atoi(argv[i] + 2);
                    } else if (i + 1 < argc) {
                        options->directGainVal = atoi(argv[i + 1]);
                        i++;
                        options->fileStart++;
                    } else {
                        errUsage(argv[0]);
                    }
                    break;
                case 'h':
                case 'H':
                case '?':
                    if ((argv[i][2] == 'w') || (argv[i][2] == 'W')) {
                        wrapExplanation();
                    } else if (i + 1 < argc) {
                        if ((argv[i + 1][0] == 'w') || (argv[i + 1][0] == 'W')) {
                            wrapExplanation();
                        }
                    } else {
                        fullUsage(argv[0]);
                    }
                    fullUsage(argv[0]);
                    break;
                case 'k':
                case 'K':
                    options->autoClip = !0;
                    break;
                case 'l':
                case 'L':
                    options->directSingleChannelGain = !0;
                    options->directGain = 0;
                    if (argv[i][2] != '\0') {
                        whichChannel = atoi(argv[i] + 2);
                        if (i + 1 < argc) {
                            options->directGainVal = atoi(argv[i + 1]);
                            i++;
                            options->fileStart++;
                        } else {
                            errUsage(argv[0]);
                        }
                    } else if (i + 2 < argc) {
                        whichChannel = atoi(argv[i + 1]);
                        i++;
                        options->fileStart++;
                        options->directGainVal = atoi(argv[i + 1]);
                        i++;
                        options->fileStart++;
                    } else {
                        errUsage(argv[0]);
                    }
                    break;
                case 'm':
                case 'M':
                    if (argv[i][2] != '\0') {
                        options->mp3GainMod = atoi(argv[i] + 2);
                    } else if (i + 1 < argc) {
                        options->mp3GainMod = atoi(argv[i + 1]);
                        i++;
                        options->fileStart++;
                    } else {
                        errUsage(argv[0]);
                    }
                    break;
                case 'o':
                case 'O':
                    options->databaseFormat = !0;
                    break;
                case 'p':
                case 'P':
                    saveTime = !0;
                    break;
                case 'q':
                case 'Q':
                    QuietMode = !0;
                    break;
                case 'r':
                case 'R':
                    options->applyTrack = !0;
                    options->applyAlbum = 0;
                    break;
                case 's':
                case 'S':
                    chtmp = 0;
                    if (argv[i][2] == '\0') {
                        if (i + 1 < argc) {
                            i++;
                            options->fileStart++;
                            chtmp = argv[i][0];
                        } else {
                            errUsage(argv[0]);
                        }
                    } else {
                        chtmp = argv[i][2];
                    }
                    switch (chtmp) {
                        case 'c':
                        case 'C':
                            checkTagOnly = !0;
                            break;
                        case 'd':
                        case 'D':
                            deleteTag = !0;
                            break;
                        case 's':
                        case 'S':
                            skipTag = !0;
                            break;
                        case 'u':
                        case 'U':
                            forceUpdateTag = !0;
                            break;
                        case 'r':
                        case 'R':
                            forceRecalculateTag = !0;
                            break;
                        case 'i':
                        case 'I':
                            useId3 = 1;
                            break;
                        case 'a':
                        case 'A':
                            useId3 = 0;
                            break;
                        default:
                            errUsage(argv[0]);
                    }
                    break;
                case 't':
                    UsingTemp = !0;
                    break;
                case 'T':
                    UsingTemp = 0;
                    break;
                case 'u':
                case 'U':
                    undoChanges = !0;
                    break;
                case 'v':
                case 'V':
                    showVersion(argv[0]);
                    fclose(stdout);
                    fclose(stderr);
                    exit(0);
                case 'w':
                case 'W':
                    wrapGain = !0;
                    break;
                case 'x':
                case 'X':
                    maxAmpOnly = !0;
                    break;
                case 'e':
                case 'E':
                    options->analysisTrack = !0;
                    break;
                default:
                    fprintf(stderr, "I don't recognize option %s\n", argv[i]);
            }
        }
    }
}

void mp3gain_cli_print_table_header(int databaseFormat, int checkTagOnly, int undoChanges)
{
    if (!databaseFormat) {
        return;
    }

    if (checkTagOnly) {
        fprintf(stdout, "File\tMP3 gain\tdB gain\tMax Amplitude\tMax global_gain\tMin global_gain\tAlbum gain\tAlbum dB gain\tAlbum Max Amplitude\tAlbum Max global_gain\tAlbum Min global_gain\n");
    } else if (undoChanges) {
        fprintf(stdout, "File\tleft global_gain change\tright global_gain change\n");
    } else {
        fprintf(stdout, "File\tMP3 gain\tdB gain\tMax Amplitude\tMax global_gain\tMin global_gain\n");
    }
    fflush(stdout);
}


