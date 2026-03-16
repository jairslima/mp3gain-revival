#ifndef MP3GAIN_EXEC_H
#define MP3GAIN_EXEC_H

#include "apetag.h"

#ifdef AACGAIN
#include "aacgain.h"
#endif

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
);

#endif
