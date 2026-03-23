#include <math.h>
#include <stdio.h>

#include "apetag.h"
#include "exec.h"
#include "id3tag.h"

#ifdef AACGAIN
#define AACGAIN_ARG(x) , x
#else
#define AACGAIN_ARG(x)
#endif

#include "mp3gain_config.h"

extern int gSuccess;
int changeGain(char *filename AACGAIN_ARG(AACGainHandle aacH), int leftgainchange, int rightgainchange);
void changeGainAndTag(char *filename AACGAIN_ARG(AACGainHandle aacH), int leftgainchange, int rightgainchange, struct MP3GainTagInfo *tag, struct FileTagsStruct *fileTag);

static int mp3gain_round_gain(double gain)
{
    if (fabs(gain) - (double)((int)(fabs(gain))) < 0.5) {
        return (int)(gain);
    }

    return (int)(gain) + (gain < 0 ? -1 : 1);
}

static void mp3gain_print_tag_only(char *filename, struct MP3GainTagInfo *tagInfo, int databaseFormat)
{
    int intGainChange = 0;
    int intAlbumGainChange = 0;
    double dblGainChange;

    if (tagInfo->haveTrackGain) {
        dblGainChange = tagInfo->trackGain / (5.0 * log10(2.0));
        intGainChange = mp3gain_round_gain(dblGainChange);
    }

    if (tagInfo->haveAlbumGain) {
        dblGainChange = tagInfo->albumGain / (5.0 * log10(2.0));
        intAlbumGainChange = mp3gain_round_gain(dblGainChange);
    }

    if (!databaseFormat) {
        fprintf(stdout, "%s\n", filename);
        if (tagInfo->haveTrackGain) {
            fprintf(stdout, "Recommended \"Track\" dB change: %f\n", tagInfo->trackGain);
            fprintf(stdout, "Recommended \"Track\" mp3 gain change: %d\n", intGainChange);
            if (tagInfo->haveTrackPeak && tagInfo->trackPeak * pow(2.0, (double)(intGainChange) / 4.0) > 1.0) {
                fprintf(stdout, "WARNING: some clipping may occur with this gain change!\n");
            }
        }
        if (tagInfo->haveTrackPeak) {
            fprintf(stdout, "Max PCM sample at current gain: %f\n", tagInfo->trackPeak * 32768.0);
        }
        if (tagInfo->haveMinMaxGain) {
            fprintf(stdout, "Max mp3 global gain field: %d\n", tagInfo->maxGain);
            fprintf(stdout, "Min mp3 global gain field: %d\n", tagInfo->minGain);
        }
        if (tagInfo->haveAlbumGain) {
            fprintf(stdout, "Recommended \"Album\" dB change: %f\n", tagInfo->albumGain);
            fprintf(stdout, "Recommended \"Album\" mp3 gain change: %d\n", intAlbumGainChange);
            if (tagInfo->haveTrackPeak && tagInfo->trackPeak * pow(2.0, (double)(intAlbumGainChange) / 4.0) > 1.0) {
                fprintf(stdout, "WARNING: some clipping may occur with this gain change!\n");
            }
        }
        if (tagInfo->haveAlbumPeak) {
            fprintf(stdout, "Max Album PCM sample at current gain: %f\n", tagInfo->albumPeak * 32768.0);
        }
        if (tagInfo->haveAlbumMinMaxGain) {
            fprintf(stdout, "Max Album mp3 global gain field: %d\n", tagInfo->albumMaxGain);
            fprintf(stdout, "Min Album mp3 global gain field: %d\n", tagInfo->albumMinGain);
        }
        fprintf(stdout, "\n");
        return;
    }

    fprintf(stdout, "%s\t", filename);
    if (tagInfo->haveTrackGain) {
        fprintf(stdout, "%d\t%f\t", intGainChange, tagInfo->trackGain);
    } else {
        fprintf(stdout, "NA\tNA\t");
    }
    if (tagInfo->haveTrackPeak) {
        fprintf(stdout, "%f\t", tagInfo->trackPeak * 32768.0);
    } else {
        fprintf(stdout, "NA\t");
    }
    if (tagInfo->haveMinMaxGain) {
        fprintf(stdout, "%d\t%d\t", tagInfo->maxGain, tagInfo->minGain);
    } else {
        fprintf(stdout, "NA\tNA\t");
    }
    if (tagInfo->haveAlbumGain) {
        fprintf(stdout, "%d\t%f\t", intAlbumGainChange, tagInfo->albumGain);
    } else {
        fprintf(stdout, "NA\tNA\t");
    }
    if (tagInfo->haveAlbumPeak) {
        fprintf(stdout, "%f\t", tagInfo->albumPeak * 32768.0);
    } else {
        fprintf(stdout, "NA\t");
    }
    if (tagInfo->haveAlbumMinMaxGain) {
        fprintf(stdout, "%d\t%d\n", tagInfo->albumMaxGain, tagInfo->albumMinGain);
    } else {
        fprintf(stdout, "NA\tNA\n");
    }
    fflush(stdout);
}

int mp3gain_handle_simple_action(
    char *filename,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int databaseFormat,
    int *directGain,
    int directSingleChannelGain,
    int directGainVal,
    int *gSuccess
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
)
{
    if (g_mp3gain_config.checkTagOnly) {
        mp3gain_print_tag_only(filename, tagInfo, databaseFormat);
        return 1;
    }

    if (g_mp3gain_config.undoChanges) {
        *directGain = !0;
        if (tagInfo->haveUndo && (tagInfo->undoLeft || tagInfo->undoRight)) {
            if ((!g_mp3gain_config.QuietMode) && (!databaseFormat)) {
                fprintf(stderr, "Undoing mp3gain changes (%d,%d) to %s...\n", tagInfo->undoLeft, tagInfo->undoRight, filename);
            }
            if (databaseFormat) {
                fprintf(stdout, "%s\t%d\t%d\n", filename, tagInfo->undoLeft, tagInfo->undoRight);
            }
            changeGainAndTag(filename AACGAIN_ARG(aacH), tagInfo->undoLeft, tagInfo->undoRight, tagInfo, fileTags);
        } else {
            if (databaseFormat) {
                fprintf(stdout, "%s\t0\t0\n", filename);
            } else if (!g_mp3gain_config.QuietMode) {
                if (tagInfo->haveUndo) {
                    fprintf(stderr, "No changes to undo in %s\n", filename);
                } else {
                    fprintf(stderr, "No undo information in %s\n", filename);
                }
                *gSuccess = 0;
            }
        }
        return 1;
    }

    if (directSingleChannelGain) {
        if (!g_mp3gain_config.QuietMode) {
            fprintf(stderr, "Applying gain change of %d to CHANNEL %d of %s...\n", directGainVal, g_mp3gain_config.whichChannel, filename);
        }
        if (g_mp3gain_config.whichChannel) {
            if (g_mp3gain_config.skipTag) {
                changeGain(filename AACGAIN_ARG(aacH), 0, directGainVal);
            } else {
                changeGainAndTag(filename AACGAIN_ARG(aacH), 0, directGainVal, tagInfo, fileTags);
            }
        } else {
            if (g_mp3gain_config.skipTag) {
                changeGain(filename AACGAIN_ARG(aacH), directGainVal, 0);
            } else {
                changeGainAndTag(filename AACGAIN_ARG(aacH), directGainVal, 0, tagInfo, fileTags);
            }
        }
        if ((!g_mp3gain_config.QuietMode) && (*gSuccess == 1)) {
            fprintf(stderr, "\ndone\n");
        }
        return 1;
    }

    if (*directGain) {
        if (!g_mp3gain_config.QuietMode) {
            fprintf(stderr, "Applying gain change of %d to %s...\n", directGainVal, filename);
        }
        if (g_mp3gain_config.skipTag) {
            changeGain(filename AACGAIN_ARG(aacH), directGainVal, directGainVal);
        } else {
            changeGainAndTag(filename AACGAIN_ARG(aacH), directGainVal, directGainVal, tagInfo, fileTags);
        }
        if ((!g_mp3gain_config.QuietMode) && (*gSuccess == 1)) {
            fprintf(stderr, "\ndone\n");
        }
        return 1;
    }

    if (g_mp3gain_config.deleteTag) {
#ifdef AACGAIN
        if (aacH) {
            aac_clear_rg_tags(aacH);
        } else
#endif
        {
            RemoveMP3GainAPETag(filename, g_mp3gain_config.saveTime);
            if (g_mp3gain_config.useId3) {
                RemoveMP3GainID3Tag(filename, g_mp3gain_config.saveTime);
            }
        }
        if ((!g_mp3gain_config.QuietMode) && (!databaseFormat)) {
            fprintf(stderr, "Deleting tag info of %s...\n", filename);
        }
        if (databaseFormat) {
            fprintf(stdout, "%s\tNA\tNA\tNA\tNA\tNA\n", filename);
        }
        return 1;
    }

    return 0;
}
