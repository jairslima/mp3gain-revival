#include <stdlib.h>
#include <string.h>

#include "apetag.h"
#include "id3tag.h"
#include "mp3gain.h"
#include "prep.h"

extern int UsingTemp;
extern int skipTag;
extern int deleteTag;
extern int forceRecalculateTag;
extern int forceUpdateTag;
extern short int saveTime;
extern int useId3;

enum {
    MP3GAIN_FULL_RECALC = 1,
    MP3GAIN_AMP_RECALC = 2,
    MP3GAIN_MIN_MAX_GAIN_RECALC = 4
};

#ifdef AACGAIN
void ReadAacTags(AACGainHandle gh, struct MP3GainTagInfo *info);
#endif

static void mp3gain_reset_runtime_state(
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags)
{
    *fileok = 0;

    memset(tagInfo, 0, sizeof(*tagInfo));
    memset(fileTags, 0, sizeof(*fileTags));

    tagInfo->dirty = forceUpdateTag;
}

static void mp3gain_clear_recalculated_fields(struct MP3GainTagInfo *tagInfo)
{
    if (tagInfo->haveAlbumGain) {
        tagInfo->dirty = !0;
        tagInfo->haveAlbumGain = 0;
    }
    if (tagInfo->haveAlbumPeak) {
        tagInfo->dirty = !0;
        tagInfo->haveAlbumPeak = 0;
    }
    if (tagInfo->haveTrackGain) {
        tagInfo->dirty = !0;
        tagInfo->haveTrackGain = 0;
    }
    if (tagInfo->haveTrackPeak) {
        tagInfo->dirty = !0;
        tagInfo->haveTrackPeak = 0;
    }
    if (tagInfo->haveMinMaxGain) {
        tagInfo->dirty = !0;
        tagInfo->haveMinMaxGain = 0;
    }
    if (tagInfo->haveAlbumMinMaxGain) {
        tagInfo->dirty = !0;
        tagInfo->haveAlbumMinMaxGain = 0;
    }
}

void mp3gain_prepare_file(
    char *filename,
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
)
{
    mp3gain_reset_runtime_state(fileok, tagInfo, fileTags);

#ifdef AACGAIN
    if (aac_open(filename, UsingTemp, saveTime, aacInfo) != 0) {
        passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 2,
            filename, " is not a valid mp4/m4a file.\n");
        exit(1);
    }
#endif

    if ((!skipTag) && (!deleteTag)) {
#ifdef AACGAIN
        if (*aacInfo) {
            if (!skipTag) {
                ReadAacTags(*aacInfo, tagInfo);
            }
        } else
#endif
        {
            ReadMP3GainAPETag(filename, tagInfo, fileTags);
            if (useId3) {
                if (tagInfo->haveTrackGain || tagInfo->haveAlbumGain ||
                    tagInfo->haveMinMaxGain || tagInfo->haveAlbumMinMaxGain ||
                    tagInfo->haveUndo) {
                    tagInfo->dirty = 1;
                }
                ReadMP3GainID3Tag(filename, tagInfo);
            }
        }

        if (forceRecalculateTag) {
            mp3gain_clear_recalculated_fields(tagInfo);
        }
    }
}

int mp3gain_compute_recalc_flags(
    int argc,
    int fileStart,
    struct MP3GainTagInfo *tagInfo,
    int applyTrack,
    int analysisTrack,
    int maxAmpOnly
)
{
    int mainloop;
    int albumRecalc;
    double curAlbumGain = 0;
    double curAlbumPeak = 0;
    unsigned char curAlbumMinGain = 0;
    unsigned char curAlbumMaxGain = 0;

    albumRecalc = forceRecalculateTag || skipTag ? MP3GAIN_FULL_RECALC : 0;
    if ((!skipTag) && (!deleteTag) && (!forceRecalculateTag)) {
        if (argc - fileStart > 1) {
            curAlbumGain = tagInfo[fileStart].albumGain;
            curAlbumPeak = tagInfo[fileStart].albumPeak;
            curAlbumMinGain = tagInfo[fileStart].albumMinGain;
            curAlbumMaxGain = tagInfo[fileStart].albumMaxGain;
        }

        for (mainloop = fileStart; mainloop < argc; mainloop++) {
            if (!maxAmpOnly) {
                if (argc - fileStart > 1 && !applyTrack && !analysisTrack) {
                    if (!tagInfo[mainloop].haveAlbumGain) {
                        albumRecalc |= MP3GAIN_FULL_RECALC;
                    } else if (tagInfo[mainloop].albumGain != curAlbumGain) {
                        albumRecalc |= MP3GAIN_FULL_RECALC;
                    }
                }
                if (!tagInfo[mainloop].haveTrackGain) {
                    tagInfo[mainloop].recalc |= MP3GAIN_FULL_RECALC;
                }
            }

            if (argc - fileStart > 1 && !applyTrack && !analysisTrack) {
                if (!tagInfo[mainloop].haveAlbumPeak) {
                    albumRecalc |= MP3GAIN_AMP_RECALC;
                } else if (tagInfo[mainloop].albumPeak != curAlbumPeak) {
                    albumRecalc |= MP3GAIN_AMP_RECALC;
                }
                if (!tagInfo[mainloop].haveAlbumMinMaxGain) {
                    albumRecalc |= MP3GAIN_MIN_MAX_GAIN_RECALC;
                } else if (tagInfo[mainloop].albumMaxGain != curAlbumMaxGain) {
                    albumRecalc |= MP3GAIN_MIN_MAX_GAIN_RECALC;
                } else if (tagInfo[mainloop].albumMinGain != curAlbumMinGain) {
                    albumRecalc |= MP3GAIN_MIN_MAX_GAIN_RECALC;
                }
            }

            if (!tagInfo[mainloop].haveTrackPeak) {
                tagInfo[mainloop].recalc |= MP3GAIN_AMP_RECALC;
            }
            if (!tagInfo[mainloop].haveMinMaxGain) {
                tagInfo[mainloop].recalc |= MP3GAIN_MIN_MAX_GAIN_RECALC;
            }
        }
    }

    return albumRecalc;
}
