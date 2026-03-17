#ifndef MP3GAIN_PROCESS_H
#define MP3GAIN_PROCESS_H

#include <stdio.h>

#include "apetag.h"
#include "gain_analysis.h"
#include <mpg123.h>

#ifdef AACGAIN
#include "aacgain.h"
#endif

int mp3gain_open_input_file(
    char *filename,
    int recalc,
    long *gFilesize,
    FILE **inf,
    int *gSuccess
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
);

int mp3gain_init_decoder(mpg123_handle **mh, int *decodeSuccess);

void mp3gain_prepare_runtime_batch(
    int argc,
    char **argv,
    int fileStart,
    int databaseFormat,
    int **fileok,
    struct MP3GainTagInfo **tagInfo,
    struct FileTagsStruct **fileTags
#ifdef AACGAIN
    , AACGainHandle **aacInfo
#endif
);

void mp3gain_prepare_files_batch(
    int argc,
    char **argv,
    int fileStart,
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

int mp3gain_run_batch(
    char **argv,
    int argc,
    int fileStart,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int *fileok,
    int maxAmpOnly,
    double dBGainMod,
    int mp3GainMod,
    int applyTrack,
    int analysisTrack,
    int applyAlbum,
    int databaseFormat,
    int saveTime,
    int skipTag,
    int autoClip,
    int ignoreClipWarning,
    int *directGain,
    int directSingleChannelGain,
    int directGainVal,
    int *gSuccess,
    int *first,
    int *numFiles,
    Float_t *dBchange,
    int *intGainChange,
    int deleteTag,
    int checkTagOnly
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

void mp3gain_load_existing_track_data(
    struct MP3GainTagInfo *tagInfo,
    Float_t *maxsample,
    unsigned char *maxgain,
    unsigned char *mingain,
    unsigned long *ok
);

void mp3gain_prepare_recalc_state(
    struct MP3GainTagInfo *tagInfo,
    Float_t *maxsample
);

unsigned long mp3gain_begin_mp3_scan(
    unsigned char *maxgain,
    unsigned char *mingain
);

unsigned long mp3gain_prime_mp3_frames(int recalc);

int mp3gain_handle_missing_mp3_frames(
    unsigned long ok,
    int *gSuccess,
    char *filename
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
);

void mp3gain_mark_valid_file(
    int *fileok,
    int *numFiles,
    int recalc
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
);

void mp3gain_sync_mp3_frequency(
    int mpegver,
    int freqidx,
    int maxAmpOnly,
    int *first,
    double *lastfreq,
    char *analysisError,
    const double frequency[4][4]
);

unsigned long mp3gain_prepare_first_mp3_frame(
    unsigned char *curframe,
    char *filename,
    int *mode,
    int *mpegver,
    int *freqidx,
    unsigned long *frame,
    long arrbytesinframe[16]
);

int mp3gain_finalize_track_analysis(
    struct MP3GainTagInfo *curTag,
    int recalc,
    int maxAmpOnly,
    double dBGainMod,
    int mp3GainMod,
    Float_t maxsample,
    unsigned char mingain,
    unsigned char maxgain,
    Float_t *dBchange,
    int *intGainChange
);

int mp3gain_compute_gain_change(
    Float_t dBchange,
    int mp3GainMod
);

int mp3gain_report_track_result(
    const char *filename,
    int databaseFormat,
    int applyTrack,
    int applyAlbum,
    Float_t dBchange,
    int intGainChange,
    Float_t maxsample,
    unsigned char maxgain,
    unsigned char mingain
);

int mp3gain_report_album_result(
    char **argv,
    int fileStart,
    int argc,
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    int databaseFormat,
    int applyAlbum,
    Float_t dBchange,
    int intGainChange,
    Float_t maxmaxsample,
    unsigned char maxmaxgain,
    unsigned char minmingain
);

int mp3gain_apply_track_change(
    char *filename,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int saveTime,
    int skipTag,
    int autoClip,
    int ignoreClipWarning,
    Float_t maxsample,
    int *intGainChange,
    int *first
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
);

void mp3gain_apply_album_change(
    char **argv,
    int fileStart,
    int argc,
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int saveTime,
    int skipTag,
    int autoClip,
    int ignoreClipWarning,
    Float_t maxmaxsample,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

void mp3gain_write_dirty_tags(
    char **argv,
    int fileStart,
    int argc,
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int saveTime
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

void mp3gain_process_frame_audio(
    mpg123_handle *mh,
    long bytesinframe,
    int nchan,
    int recalc,
    int maxAmpOnly,
    int *decodeSuccess,
    int *nprocsamp,
    Float_t *lsamples,
    Float_t *rsamples,
    Float_t *maxsample,
    char *analysisError
);

unsigned long mp3gain_process_mp3_frame_iteration(
    mpg123_handle *mh,
    long bytesinframe,
    int nchan,
    int recalc,
    int maxAmpOnly,
    int *decodeSuccess,
    int *nprocsamp,
    Float_t *lsamples,
    Float_t *rsamples,
    Float_t *maxsample,
    unsigned char *maxgain,
    unsigned char *mingain,
    char *analysisError,
    unsigned long frame,
    long gFilesize
);

unsigned long mp3gain_process_mp3_frame_iteration_safe(
    mpg123_handle *mh,
    char *filename,
    long bytesinframe,
    int nchan,
    int recalc,
    int maxAmpOnly,
    int *decodeSuccess,
    int *nprocsamp,
    Float_t *lsamples,
    Float_t *rsamples,
    Float_t *maxsample,
    unsigned char *maxgain,
    unsigned char *mingain,
    char *analysisError,
    unsigned long frame,
    long gFilesize
);

unsigned long mp3gain_advance_frame_scan(
    long bytesinframe,
    char analysisError,
    unsigned long frame,
    long gFilesize
);

unsigned long mp3gain_prepare_mp3_frame(
    char *filename,
    int *bitridx,
    int *mpegver,
    int *crcflag,
    int *freqidx,
    long *bytesinframe,
    int *mode,
    int *nchan,
    long arrbytesinframe[16]
);

unsigned long mp3gain_process_mp3_frames(
    mpg123_handle *mh,
    char *filename,
    int recalc,
    int maxAmpOnly,
    int *decodeSuccess,
    int *nprocsamp,
    Float_t *lsamples,
    Float_t *rsamples,
    Float_t *maxsample,
    unsigned char *maxgain,
    unsigned char *mingain,
    char *analysisError,
    unsigned long *frame,
    int *bitridx,
    int *mpegver,
    int *crcflag,
    int *freqidx,
    long *bytesinframe,
    int *mode,
    int *nchan,
    long arrbytesinframe[16],
    long gFilesize
);

unsigned long mp3gain_run_file_recalc(
    mpg123_handle *mh,
    char *filename,
    int recalc,
    int maxAmpOnly,
    int *decodeSuccess,
    int *nprocsamp,
    Float_t *lsamples,
    Float_t *rsamples,
    Float_t *maxsample,
    unsigned char *maxgain,
    unsigned char *mingain,
    char *analysisError,
    int *first,
    double *lastfreq,
    int *numFiles,
    int *fileok,
    int *gSuccess,
    unsigned long *ok,
    unsigned long *frame,
    int *bitridx,
    int *mpegver,
    int *crcflag,
    int *freqidx,
    long *bytesinframe,
    int *mode,
    int *nchan,
    long arrbytesinframe[16],
    const double frequency[4][4],
    long gFilesize
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
);

void mp3gain_finish_track_recalc(
    char *filename,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int maxAmpOnly,
    double dBGainMod,
    int mp3GainMod,
    Float_t maxsample,
    unsigned char mingain,
    unsigned char maxgain,
    int databaseFormat,
    int applyTrack,
    int applyAlbum,
    int saveTime,
    int skipTag,
    int autoClip,
    int ignoreClipWarning,
    int *first,
    double *lastfreq,
    int *gSuccess,
    int *numFiles,
    Float_t *dBchange,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
);

void mp3gain_finish_album_analysis(
    char **argv,
    int argc,
    int fileStart,
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int albumRecalc,
    int maxAmpOnly,
    double dBGainMod,
    int mp3GainMod,
    int databaseFormat,
    int applyAlbum,
    int skipTag,
    int saveTime,
    int autoClip,
    int ignoreClipWarning,
    int numFiles,
    Float_t *dBchange,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

void mp3gain_process_file_analysis(
    char *filename,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int *fileok,
    int maxAmpOnly,
    double dBGainMod,
    int mp3GainMod,
    int databaseFormat,
    int applyTrack,
    int applyAlbum,
    int saveTime,
    int skipTag,
    int autoClip,
    int ignoreClipWarning,
    int *first,
    double *lastfreq,
    int *gSuccess,
    int *numFiles,
    Float_t *dBchange,
    int *intGainChange,
    long *gFilesize,
    FILE **inf,
    mpg123_handle **mh,
    int *decodeSuccess,
    unsigned long *ok,
    int *nprocsamp,
    Float_t *lsamples,
    Float_t *rsamples,
    Float_t *maxsample,
    unsigned char *maxgain,
    unsigned char *mingain,
    char *analysisError,
    unsigned long *frame,
    int *bitridx,
    int *mpegver,
    int *crcflag,
    int *freqidx,
    long *bytesinframe,
    int *mode,
    int *nchan,
    long arrbytesinframe[16],
    const double frequency[4][4]
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
);

void mp3gain_process_files_batch(
    int argc,
    char **argv,
    int fileStart,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int *fileok,
    int albumRecalc,
    int databaseFormat,
    int maxAmpOnly,
    double dBGainMod,
    int mp3GainMod,
    int applyTrack,
    int applyAlbum,
    int saveTime,
    int skipTag,
    int autoClip,
    int ignoreClipWarning,
    int directSingleChannelGain,
    int directGainVal,
    int *directGain,
    int *gSuccess,
    int *first,
    int *numFiles,
    Float_t *dBchange,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

void mp3gain_free_runtime_state(
    int argc,
    int fileStart,
    struct MP3GainTagInfo *tagInfo,
    int *fileok,
    struct FileTagsStruct *fileTags
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

int mp3gain_finalize_batch(
    char **argv,
    int argc,
    int fileStart,
    int *fileok,
    struct MP3GainTagInfo *tagInfo,
    struct FileTagsStruct *fileTags,
    int albumRecalc,
    int maxAmpOnly,
    double dBGainMod,
    int mp3GainMod,
    int databaseFormat,
    int applyTrack,
    int analysisTrack,
    int applyAlbum,
    int directGain,
    int directSingleChannelGain,
    int deleteTag,
    int skipTag,
    int checkTagOnly,
    int saveTime,
    int autoClip,
    int ignoreClipWarning,
    int numFiles,
    int gSuccess,
    Float_t *dBchange,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
);

#ifdef AACGAIN
unsigned long mp3gain_process_aac_recalc(
    AACGainHandle aacH,
    int maxAmpOnly,
    Float_t *maxsample,
    unsigned char *mingain,
    unsigned char *maxgain,
    int *first,
    double *lastfreq,
    char *analysisError,
    int *numFiles,
    char *filename
);
#endif

#endif
