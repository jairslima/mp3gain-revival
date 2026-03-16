#ifndef MP3GAIN_PREP_H
#define MP3GAIN_PREP_H

#include "apetag.h"

#ifdef AACGAIN
#include "aacgain.h"
#endif

void mp3gain_prepare_file(
    char *filename,
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

int mp3gain_compute_recalc_flags(
    int argc,
    int fileStart,
    struct MP3GainTagInfo *tagInfo,
    int applyTrack,
    int analysisTrack,
    int maxAmpOnly
);

#endif
