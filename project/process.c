#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "apetag.h"
#include "cli.h"
#include "exec.h"
#include "gain_analysis.h"
#include "mp3gain.h"
#include "prep.h"
#include <mpg123.h>

#include "process.h"
#include "mp3gain_config.h"

enum {
    MP3GAIN_FULL_RECALC = 1,
    MP3GAIN_AMP_RECALC = 2
};

extern int totFiles;
extern const double frequency[4][4];
extern FILE *inf;

int getSizeOfFile(char *filename);
unsigned long fillBuffer(long savelastbytes);
unsigned long reportPercentAnalyzed(unsigned long percent, unsigned long bytes);
void WriteMP3GainTag(char *filename
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
    , struct MP3GainTagInfo *info, struct FileTagsStruct *fileTags, int saveTimeStamp);
void changeGainAndTag(char *filename
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
    , int leftgainchange, int rightgainchange, struct MP3GainTagInfo *tag, struct FileTagsStruct *fileTag);
int changeGain(char *filename
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
    , int leftgainchange, int rightgainchange);
int queryUserForClipping(char *argv_mainloop, int intGainChange);
void convert_decout(float *decout, int nprocsamp, int nchan, Float_t *lsamples, Float_t *rsamples);
float find_maxsample(float *decout, int nsamples, float maxsample);

int mp3gain_open_input_file(
    char *filename,
    int recalc,
    long *gFilesize,
    FILE **inf,
    int *gSuccess
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
)
{
    if (recalc > 0) {
        *gFilesize = getSizeOfFile(filename);
#ifdef AACGAIN
        if (!aacH)
#endif
        {
            *inf = fopen(filename, "rb");
        }
    }

#ifdef AACGAIN
    if (!aacH && (*inf == NULL) && (recalc > 0)) {
#else
    if ((*inf == NULL) && (recalc > 0)) {
#endif
        fprintf(stderr, "Can't open %s for reading\n", filename);
        fflush(stderr);
        *gSuccess = 0;
        return 0;
    }

    return 1;
}

int mp3gain_init_decoder(mpg123_handle **mh, int *decodeSuccess)
{
    if (
        !(*mh = mpg123_new(NULL, decodeSuccess)) ||
        MPG123_OK != mpg123_param(*mh, MPG123_ADD_FLAGS, MPG123_FORCE_FLOAT, 0.) ||
        MPG123_OK != mpg123_param(*mh, MPG123_REMOVE_FLAGS, MPG123_GAPLESS | MPG123_SEEKBUFFER, 0.) ||
        MPG123_OK != mpg123_param(*mh, MPG123_VERBOSE, 0, 0.) ||
#if (MPG123_API_VERSION >= 45)
        MPG123_OK != mpg123_param(*mh, MPG123_ADD_FLAGS, MPG123_NO_READAHEAD, 0.) ||
#endif
        MPG123_OK != mpg123_open_feed(*mh)
    ) {
        fprintf(stderr, "Failed to create/setup mpg123 handle: %s\n",
            *mh ? mpg123_strerror(*mh) : mpg123_plain_strerror(*decodeSuccess));
        return 0;
    }

    return 1;
}

int mp3gain_prepare_runtime_batch(
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
)
{
    *fileok = (int *)malloc(sizeof(int) * argc);
    *tagInfo = (struct MP3GainTagInfo *)calloc(argc, sizeof(struct MP3GainTagInfo));
    *fileTags = (struct FileTagsStruct *)malloc(sizeof(struct FileTagsStruct) * argc);
#ifdef AACGAIN
    *aacInfo = (AACGainHandle *)malloc(sizeof(AACGainHandle) * argc);
#endif

    if (!(*fileok) || !(*tagInfo) || !(*fileTags)
#ifdef AACGAIN
        || !(*aacInfo)
#endif
    ) {
        fprintf(stderr, "Out of memory while preparing runtime state.\n");
        free(*fileok);
        free(*tagInfo);
        free(*fileTags);
        *fileok = NULL;
        *tagInfo = NULL;
        *fileTags = NULL;
#ifdef AACGAIN
        free(*aacInfo);
        *aacInfo = NULL;
#endif
        return 0;
    }

    mp3gain_cli_print_table_header(databaseFormat, g_mp3gain_config.checkTagOnly, g_mp3gain_config.undoChanges);
    totFiles = argc - fileStart;
    mp3gain_prepare_files_batch(
        argc, argv, fileStart, *fileok, *tagInfo, *fileTags
#ifdef AACGAIN
            , *aacInfo
#endif
    );
    return 1;
}

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
)
{
    int mainloop;

    for (mainloop = fileStart; mainloop < argc; mainloop++) {
        mp3gain_prepare_file(
            argv[mainloop],
            &fileok[mainloop],
            &tagInfo[mainloop],
            &fileTags[mainloop]
#ifdef AACGAIN
            , &aacInfo[mainloop]
#endif
        );
    }
}

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
)
{
    int albumRecalc;
    struct MP3GainBatchContext batch = { gSuccess, first, numFiles, &g_mp3gain_config.lastfreq };

    albumRecalc = mp3gain_compute_recalc_flags(
        argc, fileStart, tagInfo, applyTrack, analysisTrack, maxAmpOnly
    );

    mp3gain_process_files_batch(
        argc, argv, fileStart, tagInfo, fileTags, fileok, albumRecalc,
        databaseFormat, maxAmpOnly, dBGainMod, mp3GainMod, applyTrack,
        applyAlbum, saveTime, skipTag, autoClip, ignoreClipWarning,
        directSingleChannelGain, directGainVal, directGain, &batch, dBchange, intGainChange
#ifdef AACGAIN
        , aacInfo
#endif
    );

    return mp3gain_finalize_batch(
        argv, argc, fileStart, fileok, tagInfo, fileTags, albumRecalc,
        maxAmpOnly, dBGainMod, mp3GainMod, databaseFormat, applyTrack,
        analysisTrack, applyAlbum, *directGain, directSingleChannelGain,
        deleteTag, skipTag, checkTagOnly, saveTime, autoClip,
        ignoreClipWarning, *numFiles, *gSuccess, dBchange, intGainChange
#ifdef AACGAIN
        , aacInfo
#endif
    );
}

void mp3gain_load_existing_track_data(
    struct MP3GainTagInfo *tagInfo,
    Float_t *maxsample,
    unsigned char *maxgain,
    unsigned char *mingain,
    unsigned long *ok
)
{
    *maxsample = tagInfo->trackPeak * 32767.0;
    *maxgain = tagInfo->maxGain;
    *mingain = tagInfo->minGain;
    *ok = !0;
}

void mp3gain_prepare_recalc_state(
    struct MP3GainTagInfo *tagInfo,
    Float_t *maxsample
)
{
    if (!((tagInfo->recalc & MP3GAIN_FULL_RECALC) || (tagInfo->recalc & MP3GAIN_AMP_RECALC))) {
        *maxsample = tagInfo->trackPeak * 32768.0;
    } else {
        *maxsample = 0;
    }
}

unsigned long mp3gain_begin_mp3_scan(
    unsigned char *maxgain,
    unsigned char *mingain
)
{
    g_mp3gain_config.BadLayer = 0;
    g_mp3gain_config.LayerSet = g_mp3gain_config.Reckless;
    *maxgain = 0;
    *mingain = 255;
    mp3gain_reset_mp3_scan_state();
    return fillBuffer(0);
}

unsigned long mp3gain_prime_mp3_frames(int recalc)
{
    unsigned long ok = !0;

    if (recalc > 0) {
        mp3gain_prime_mp3_buffer_pointer();
        ok = mp3gain_skip_id3_and_find_first_frame();
    }

    return ok;
}

int mp3gain_handle_missing_mp3_frames(
    unsigned long ok,
    int *gSuccess,
    char *filename
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
)
{
#ifdef AACGAIN
    if (!ok && !aacH) {
#else
    if (!ok) {
#endif
        if (!g_mp3gain_config.BadLayer) {
            fprintf(stderr, "Can't find any valid MP3 frames in file %s\n", filename);
            fflush(stderr);
            *gSuccess = 0;
        }
        return 1;
    }

    return 0;
}

void mp3gain_mark_valid_file(
    int *fileok,
    int *numFiles,
    int recalc
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
)
{
    g_mp3gain_config.LayerSet = 1;
    *fileok = !0;
#ifdef AACGAIN
    if (!aacH || (recalc == 0))
#endif
    {
        (*numFiles)++;
    }
}

void mp3gain_sync_mp3_frequency(
    int mpegver,
    int freqidx,
    int maxAmpOnly,
    struct MP3GainBatchContext *batch,
    char *analysisError,
    const double frequency[4][4]
)
{
    if (!maxAmpOnly) {
        if (*(batch->first)) {
            *(batch->lastfreq) = frequency[mpegver][freqidx];
            InitGainAnalysis((long)(*(batch->lastfreq) * 1000.0));
            *analysisError = 0;
            *(batch->first) = 0;
        } else if (frequency[mpegver][freqidx] != *(batch->lastfreq)) {
            *(batch->lastfreq) = frequency[mpegver][freqidx];
            ResetSampleFrequency((long)(*(batch->lastfreq) * 1000.0));
        }
    } else {
        *analysisError = 0;
    }
}

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
)
{
    if (recalc & MP3GAIN_FULL_RECALC) {
        if (maxAmpOnly) {
            *dBchange = 0;
        } else {
            *dBchange = GetTitleGain();
        }
    } else {
        *dBchange = curTag->trackGain;
    }

    if (*dBchange == GAIN_NOT_ENOUGH_SAMPLES) {
        return 0;
    }

    if (!maxAmpOnly) {
        if (!curTag->haveTrackGain ||
            (curTag->haveTrackGain && (fabs(*dBchange - curTag->trackGain) >= 0.01))) {
            curTag->dirty = !0;
            curTag->haveTrackGain = 1;
            curTag->trackGain = *dBchange;
        }
    }

    if (!curTag->haveMinMaxGain ||
        (curTag->haveMinMaxGain && (curTag->minGain != mingain || curTag->maxGain != maxgain))) {
        curTag->dirty = !0;
        curTag->haveMinMaxGain = !0;
        curTag->minGain = mingain;
        curTag->maxGain = maxgain;
    }

    if (!curTag->haveTrackPeak ||
        (curTag->haveTrackPeak && (fabs(maxsample - (curTag->trackPeak) * 32768.0) >= 3.3))) {
        curTag->dirty = !0;
        curTag->haveTrackPeak = !0;
        curTag->trackPeak = maxsample / 32768.0;
    }

    *dBchange += dBGainMod;
    *intGainChange = mp3gain_compute_gain_change(*dBchange, mp3GainMod);

    return 1;
}

int mp3gain_compute_gain_change(
    Float_t dBchange,
    int mp3GainMod
)
{
    double dblGainChange;
    int intGainChange;

    dblGainChange = dBchange / (5.0 * log10(2.0));

    if (fabs(dblGainChange) - (double)((int)(fabs(dblGainChange))) < 0.5) {
        intGainChange = (int)(dblGainChange);
    } else {
        intGainChange = (int)(dblGainChange) + (dblGainChange < 0 ? -1 : 1);
    }

    return intGainChange + mp3GainMod;
}

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
)
{
    if (databaseFormat) {
        fprintf(stdout, "%s\t%d\t%f\t%f\t%d\t%d\n",
            filename, intGainChange, dBchange, maxsample, maxgain, mingain);
        fflush(stdout);
    }

    if ((!applyTrack) && (!applyAlbum)) {
        if (!databaseFormat) {
            fprintf(stdout, "Recommended \"Track\" dB change: %f\n", dBchange);
            fprintf(stdout, "Recommended \"Track\" mp3 gain change: %d\n", intGainChange);
            if (maxsample * (Float_t)(pow(2.0, (double)(intGainChange) / 4.0)) > 32767.0) {
                fprintf(stdout, "WARNING: some clipping may occur with this gain change!\n");
            }
            fprintf(stdout, "Max PCM sample at current gain: %f\n", maxsample);
            fprintf(stdout, "Max mp3 global gain field: %d\n", maxgain);
            fprintf(stdout, "Min mp3 global gain field: %d\n", mingain);
            fprintf(stdout, "\n");
        }
        return 1;
    }

    return 0;
}

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
)
{
    int mainloop;

    if (databaseFormat) {
        fprintf(stdout, "\"Album\"\t%d\t%f\t%f\t%d\t%d\n",
            intGainChange, dBchange, maxmaxsample * 32768.0, maxmaxgain, minmingain);
        fflush(stdout);
    }

    if (!applyAlbum) {
        if (!databaseFormat) {
            fprintf(stdout, "\nRecommended \"Album\" dB change for all files: %f\n", dBchange);
            fprintf(stdout, "Recommended \"Album\" mp3 gain change for all files: %d\n", intGainChange);
            for (mainloop = fileStart; mainloop < argc; mainloop++) {
                if (fileok[mainloop]) {
                    if (tagInfo[mainloop].trackPeak * (Float_t)(pow(2.0, (double)(intGainChange) / 4.0)) > 1.0) {
                        fprintf(stdout, "WARNING: with this global gain change, some clipping may occur in file %s\n",
                            argv[mainloop]);
                    }
                }
            }
        }
        return 1;
    }

    return 0;
}

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
)
{
    int goAhead = !0;

    *first = !0;

    if (*intGainChange == 0) {
        fprintf(stdout, "No changes to %s are necessary\n", filename);
        if (!skipTag && tagInfo->dirty) {
            fprintf(stdout, "...but tag needs update: Writing tag information for %s\n", filename);
            WriteMP3GainTag(filename
#ifdef AACGAIN
                , aacH
#endif
                , tagInfo, fileTags, saveTime);
        }
        return 1;
    }

    if (autoClip) {
        int intMaxNoClipGain = (int)(floor(4.0 * log10(32767.0 / maxsample) / log10(2.0)));
        if (*intGainChange > intMaxNoClipGain) {
            fprintf(stdout, "Applying auto-clipped mp3 gain change of %d to %s\n(Original suggested gain was %d)\n",
                intMaxNoClipGain, filename, *intGainChange);
            *intGainChange = intMaxNoClipGain;
        }
    } else if (!ignoreClipWarning) {
        if (maxsample * (Float_t)(pow(2.0, (double)(*intGainChange) / 4.0)) > 32767.0) {
            if (queryUserForClipping(filename, *intGainChange)) {
                fprintf(stdout, "Applying mp3 gain change of %d to %s...\n", *intGainChange, filename);
            } else {
                goAhead = 0;
            }
        }
    }

    if (goAhead) {
        fprintf(stdout, "Applying mp3 gain change of %d to %s...\n", *intGainChange, filename);
        if (skipTag) {
            changeGain(filename
#ifdef AACGAIN
                , aacH
#endif
                , *intGainChange, *intGainChange);
        } else {
            changeGainAndTag(filename
#ifdef AACGAIN
                , aacH
#endif
                , *intGainChange, *intGainChange, tagInfo, fileTags);
        }
    } else if (!skipTag && tagInfo->dirty) {
        fprintf(stdout, "Writing tag information for %s\n", filename);
        WriteMP3GainTag(filename
#ifdef AACGAIN
            , aacH
#endif
            , tagInfo, fileTags, saveTime);
    }

    return 1;
}

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
)
{
    int mainloop;

    if (autoClip) {
        int intMaxNoClipGain = (int)(floor(-4.0 * log10(maxmaxsample) / log10(2.0)));
        if (*intGainChange > intMaxNoClipGain) {
            fprintf(stdout, "Applying auto-clipped mp3 gain change of %d to album\n(Original suggested gain was %d)\n",
                intMaxNoClipGain, *intGainChange);
            *intGainChange = intMaxNoClipGain;
        }
    }

    for (mainloop = fileStart; mainloop < argc; mainloop++) {
        if (fileok[mainloop]) {
            if (*intGainChange == 0) {
                fprintf(stdout, "\nNo changes to %s are necessary\n", argv[mainloop]);
                if (!skipTag && tagInfo[mainloop].dirty) {
                    fprintf(stdout, "...but tag needs update: Writing tag information for %s\n", argv[mainloop]);
                    WriteMP3GainTag(argv[mainloop]
#ifdef AACGAIN
                        , aacInfo[mainloop]
#endif
                        , tagInfo + mainloop, fileTags + mainloop, saveTime);
                }
            } else {
                int goAhead = !0;

                if (!ignoreClipWarning) {
                    if (tagInfo[mainloop].trackPeak * (Float_t)(pow(2.0, (double)(*intGainChange) / 4.0)) > 1.0) {
                        goAhead = queryUserForClipping(argv[mainloop], *intGainChange);
                    }
                }

                if (goAhead) {
                    fprintf(stdout, "Applying mp3 gain change of %d to %s...\n", *intGainChange, argv[mainloop]);
                    if (skipTag) {
                        changeGain(argv[mainloop]
#ifdef AACGAIN
                            , aacInfo[mainloop]
#endif
                            , *intGainChange, *intGainChange);
                    } else {
                        changeGainAndTag(argv[mainloop]
#ifdef AACGAIN
                            , aacInfo[mainloop]
#endif
                            , *intGainChange, *intGainChange, tagInfo + mainloop, fileTags + mainloop);
                    }
                } else if (!skipTag && tagInfo[mainloop].dirty) {
                    fprintf(stdout, "Writing tag information for %s\n", argv[mainloop]);
                    WriteMP3GainTag(argv[mainloop]
#ifdef AACGAIN
                        , aacInfo[mainloop]
#endif
                        , tagInfo + mainloop, fileTags + mainloop, saveTime);
                }
            }
        }
    }
}

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
)
{
    int mainloop;

    for (mainloop = fileStart; mainloop < argc; mainloop++) {
        if (fileok[mainloop] && tagInfo[mainloop].dirty) {
            WriteMP3GainTag(argv[mainloop]
#ifdef AACGAIN
                , aacInfo[mainloop]
#endif
                , tagInfo + mainloop, fileTags + mainloop, saveTime);
        }
    }
}

int mp3gain_process_frame_audio(
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
    char *analysisError,
    int *gSuccess,
    int *fileError
)
{
    if ((recalc & MP3GAIN_AMP_RECALC) || (recalc & MP3GAIN_FULL_RECALC)) {
        size_t decbytes = 0;
        unsigned char *decout;

        mp3gain_scan_current_frame_gain();
        if (MPG123_OK != mpg123_feed(mh, mp3gain_get_current_frame_pointer(), bytesinframe)) {
            fprintf(stderr, "Feeding mpg123 failed for %s: %s\n", filename, mpg123_strerror(mh));
            *gSuccess = 0;
            *fileError = 1;
            return 0;
        }

        *decodeSuccess = mpg123_decode_frame(mh, NULL, &decout, &decbytes);
        if (*decodeSuccess == MPG123_NEW_FORMAT) {
            *decodeSuccess = mpg123_decode_frame(mh, NULL, &decout, &decbytes);
        }
        if (*decodeSuccess == MPG123_OK) {
            int enc = 0;
            int channels = 0;

            mpg123_getformat(mh, NULL, &channels, &enc);
            if (enc != MPG123_ENC_FLOAT_32 || channels != nchan) {
                fprintf(stderr, "Unexpected format returned by libmpg123 for %s.\n", filename);
                *gSuccess = 0;
                *fileError = 1;
                return 0;
            }
        } else {
#if (MPG123_API_VERSION < 45)
            if (*decodeSuccess == MPG123_NEED_MORE) {
                fprintf(stderr, "Delaying a frame in decoding with old libmpg123.\n");
                decbytes = 0;
            } else {
#endif
                fprintf(stderr, "Failed to decode MPEG frame in %s: %s\n", filename, mpg123_strerror(mh));
                *gSuccess = 0;
                *fileError = 1;
                return 0;
#if (MPG123_API_VERSION < 45)
            }
#endif
        }

        *nprocsamp = decbytes / sizeof(float) / nchan;
        if (*nprocsamp > sizeof(Float_t[1152]) / sizeof(Float_t)) {
            fprintf(stderr, "Too many samples in libmpg123 output for %s.\n", filename);
            *gSuccess = 0;
            *fileError = 1;
            return 0;
        }

        convert_decout((float*)decout, *nprocsamp, nchan, lsamples, rsamples);
        *maxsample = find_maxsample((float*)decout, (*nprocsamp) * nchan, *maxsample);
    } else {
        *decodeSuccess = !MPG123_OK;
        mp3gain_scan_current_frame_gain();
    }

    if (*decodeSuccess == MPG123_OK) {
        if ((!maxAmpOnly) && (recalc & MP3GAIN_FULL_RECALC)) {
            if (AnalyzeSamples(lsamples, rsamples, *nprocsamp, nchan) == GAIN_ANALYSIS_ERROR) {
                fprintf(stderr, "Error analyzing further samples (max time reached)          \n");
                *analysisError = !0;
            }
        }
    }

    return 1;
}

unsigned long mp3gain_process_mp3_frame_iteration(
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
    int *gSuccess,
    int *fileError,
    unsigned long frame,
    long gFilesize
)
{
    if (mp3gain_can_process_frame(bytesinframe)) {
        mp3gain_set_current_frame_minmax(mingain, maxgain);
        if (!mp3gain_process_frame_audio(
            mh, filename, bytesinframe, nchan, recalc, maxAmpOnly,
            decodeSuccess, nprocsamp, lsamples, rsamples, maxsample, analysisError, gSuccess, fileError
        )) {
            return 0;
        }
        if (*analysisError) {
            return 0;
        }
    }

    return mp3gain_advance_frame_scan(bytesinframe, *analysisError, frame, gFilesize);
}

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
    int *gSuccess,
    int *fileError,
    unsigned long frame,
    long gFilesize
)
{
#ifdef WIN32
#ifndef __GNUC__
    __try {
#endif
#endif
        return mp3gain_process_mp3_frame_iteration(
            mh, filename, bytesinframe, nchan, recalc, maxAmpOnly,
            decodeSuccess, nprocsamp, lsamples, rsamples, maxsample,
            maxgain, mingain, analysisError, gSuccess, fileError, frame, gFilesize
        );
#ifdef WIN32
#ifndef __GNUC__
    }
    __except(1) {
        fprintf(stderr, "Error analyzing %s. This mp3 has some very corrupt data.\n", filename);
        *gSuccess = 0;
        *fileError = 1;
        return 0;
    }
#endif
#endif
}

unsigned long mp3gain_advance_frame_scan(
    long bytesinframe,
    char analysisError,
    unsigned long frame,
    long gFilesize
)
{
    unsigned long ok = !0;

    if (!analysisError) {
        mp3gain_advance_frame_pointer(bytesinframe);
        ok = mp3gain_find_next_mp3_frame();
    }

    mp3gain_report_frame_progress(frame, bytesinframe, gFilesize);

    return ok;
}

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
    long gFilesize,
    int *gSuccess,
    int *fileError
)
{
    unsigned long ok = !0;

    while (ok) {
        ok = mp3gain_prepare_mp3_frame(
            filename, bitridx, mpegver, crcflag, freqidx,
            bytesinframe, mode, nchan
        );

        if (ok) {
            ok = mp3gain_process_mp3_frame_iteration_safe(
                mh, filename, *bytesinframe, *nchan, recalc, maxAmpOnly,
                decodeSuccess, nprocsamp, lsamples, rsamples, maxsample,
                maxgain, mingain, analysisError, gSuccess, fileError, ++(*frame), gFilesize
            );
        }
    }

    return ok;
}

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
    struct MP3GainBatchContext *batch,
    int *fileok,
    int *fileError,
    unsigned long *ok,
    unsigned long *frame,
    int *bitridx,
    int *mpegver,
    int *crcflag,
    int *freqidx,
    long *bytesinframe,
    int *mode,
    int *nchan,
    const double frequency[4][4],
    long gFilesize
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
)
{
    *fileError = 0;

    if (recalc == 0) {
        return *ok;
    }

#ifdef AACGAIN
    if (aacH) {
        *ok = mp3gain_process_aac_recalc(
            aacH, maxAmpOnly, maxsample, mingain, maxgain,
            batch, analysisError, filename
        );
    } else
#endif
    {
        *ok = mp3gain_begin_mp3_scan(maxgain, mingain);
    }

    if (!(*ok)) {
        return *ok;
    }

    *ok = mp3gain_prime_mp3_frames(recalc);
    if (mp3gain_handle_missing_mp3_frames(*ok, batch->gSuccess, filename
#ifdef AACGAIN
        , aacH
#endif
    )) {
        return *ok;
    }

    mp3gain_mark_valid_file(fileok, batch->numFiles, recalc
#ifdef AACGAIN
        , aacH
#endif
    );

#ifdef AACGAIN
    if (!aacH && (recalc > 0)) {
#else
    if (recalc > 0) {
#endif
        unsigned long frameLoopOk;

        *ok = mp3gain_prepare_first_mp3_frame(filename, mode, mpegver, freqidx, frame);
        mp3gain_sync_mp3_frequency(*mpegver, *freqidx, maxAmpOnly, batch, analysisError, frequency);
        frameLoopOk = mp3gain_process_mp3_frames(
            mh, filename, recalc, maxAmpOnly,
            decodeSuccess, nprocsamp, lsamples, rsamples, maxsample,
            maxgain, mingain, analysisError, frame, bitridx, mpegver,
            crcflag, freqidx, bytesinframe, mode, nchan, gFilesize, batch->gSuccess, fileError
        );
        if (*fileError) {
            *ok = 0;
        } else if (!frameLoopOk) {
            *ok = 1;
        }
        /* Normal EOF from the frame loop is not an error; leave *ok = 1. */
    }

    return *ok;
}

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
    struct MP3GainBatchContext *batch,
    Float_t *dBchange,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
)
{
    if (!g_mp3gain_config.QuietMode) {
        fprintf(stderr, "                                                 \r");
    }

    if (!mp3gain_finalize_track_analysis(
        tagInfo, tagInfo->recalc, maxAmpOnly, dBGainMod, mp3GainMod,
        maxsample, mingain, maxgain, dBchange, intGainChange
    )) {
        fprintf(stderr, "Not enough samples in %s to do analysis\n", filename);
        fflush(stderr);
        *(batch->gSuccess) = 0;
        (*(batch->numFiles))--;
        return;
    }

    if (mp3gain_report_track_result(
        filename, databaseFormat, applyTrack, applyAlbum,
        *dBchange, *intGainChange, maxsample, maxgain, mingain
    )) {
        return;
    }

    if (applyTrack) {
        *(batch->first) = !0;
        mp3gain_apply_track_change(
            filename, tagInfo, fileTags, saveTime, skipTag, autoClip,
            ignoreClipWarning, maxsample, intGainChange, batch->first
#ifdef AACGAIN
            , aacH
#endif
        );
    }
}

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
    int *gSuccess,
    Float_t *dBchange,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
)
{
    int mainloop;
    Float_t maxmaxsample;
    unsigned char maxmaxgain;
    unsigned char minmingain;

    if (albumRecalc & MP3GAIN_FULL_RECALC) {
        if (maxAmpOnly) {
            *dBchange = 0;
        } else {
            *dBchange = GetAlbumGain();
        }
    } else {
        if (tagInfo[fileStart].haveAlbumGain) {
            *dBchange = tagInfo[fileStart].albumGain;
        } else {
            *dBchange = tagInfo[fileStart].trackGain;
        }
    }

    if (*dBchange == GAIN_NOT_ENOUGH_SAMPLES) {
        fprintf(stderr, "Not enough samples in mp3 files to do analysis\n");
        fflush(stderr);
        *gSuccess = 0;
        return;
    }

    maxmaxsample = 0;
    maxmaxgain = 0;
    minmingain = 255;
    for (mainloop = fileStart; mainloop < argc; mainloop++) {
        if (fileok[mainloop]) {
            if (tagInfo[mainloop].trackPeak > maxmaxsample) {
                maxmaxsample = tagInfo[mainloop].trackPeak;
            }
            if (tagInfo[mainloop].maxGain > maxmaxgain) {
                maxmaxgain = tagInfo[mainloop].maxGain;
            }
            if (tagInfo[mainloop].minGain < minmingain) {
                minmingain = tagInfo[mainloop].minGain;
            }
        }
    }

    if ((!skipTag) && (numFiles > 1 || applyAlbum)) {
        for (mainloop = fileStart; mainloop < argc; mainloop++) {
            struct MP3GainTagInfo *curTag = tagInfo + mainloop;

            if (!maxAmpOnly) {
                if (!curTag->haveAlbumGain ||
                    (curTag->haveAlbumGain && (fabs(*dBchange - curTag->albumGain) >= 0.01))) {
                    curTag->dirty = !0;
                    curTag->haveAlbumGain = 1;
                    curTag->albumGain = *dBchange;
                }
            }

            if (!curTag->haveAlbumMinMaxGain ||
                (curTag->haveAlbumMinMaxGain &&
                 (curTag->albumMinGain != minmingain || curTag->albumMaxGain != maxmaxgain))) {
                curTag->dirty = !0;
                curTag->haveAlbumMinMaxGain = !0;
                curTag->albumMinGain = minmingain;
                curTag->albumMaxGain = maxmaxgain;
            }

            if (!curTag->haveAlbumPeak ||
                (curTag->haveAlbumPeak && (fabs(maxmaxsample - curTag->albumPeak) >= 0.0001))) {
                curTag->dirty = !0;
                curTag->haveAlbumPeak = !0;
                curTag->albumPeak = maxmaxsample;
            }
        }
    }

    *dBchange += dBGainMod;
    *intGainChange = mp3gain_compute_gain_change(*dBchange, mp3GainMod);

    if (!mp3gain_report_album_result(
        argv, fileStart, argc, fileok, tagInfo, databaseFormat,
        applyAlbum, *dBchange, *intGainChange, maxmaxsample, maxmaxgain, minmingain
    )) {
        mp3gain_apply_album_change(
            argv, fileStart, argc, fileok, tagInfo, fileTags, saveTime,
            skipTag, autoClip, ignoreClipWarning, maxmaxsample, intGainChange
#ifdef AACGAIN
            , aacInfo
#endif
        );
    }
}

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
    struct MP3GainBatchContext *batch,
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
    const double frequency[4][4]
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
)
{
    int fileError = 0;

#ifdef AACGAIN
    if (!aacH)
#endif
    {
        if (!mp3gain_init_decoder(mh, decodeSuccess)) {
            *(batch->gSuccess) = 0;
            return;
        }
    }

    if (tagInfo->recalc == 0) {
        mp3gain_load_existing_track_data(tagInfo, maxsample, maxgain, mingain, ok);
        if (*ok) {
            mp3gain_mark_valid_file(fileok, batch->numFiles, 0
#ifdef AACGAIN
                , aacH
#endif
            );
        }
    } else {
        mp3gain_prepare_recalc_state(tagInfo, maxsample);
        *ok = mp3gain_run_file_recalc(
            *mh, filename, tagInfo->recalc, maxAmpOnly, decodeSuccess,
            nprocsamp, lsamples, rsamples, maxsample, maxgain, mingain,
            analysisError, batch, fileok, &fileError, ok,
            frame, bitridx, mpegver, crcflag, freqidx, bytesinframe, mode,
            nchan, frequency, *gFilesize
#ifdef AACGAIN
            , aacH
#endif
        );
    }

    if (*ok && !fileError) {
        mp3gain_finish_track_recalc(
            filename, tagInfo, fileTags, maxAmpOnly, dBGainMod, mp3GainMod,
            *maxsample, *mingain, *maxgain, databaseFormat, applyTrack,
            applyAlbum, saveTime, skipTag, autoClip, ignoreClipWarning,
            batch, dBchange, intGainChange
#ifdef AACGAIN
            , aacH
#endif
        );
    }

#ifdef AACGAIN
    if (!aacH)
#endif
    {
        mpg123_delete(*mh);
        *mh = NULL;
    }

    fflush(stderr);
    fflush(stdout);
    if (*inf) {
        fclose(*inf);
    }
    *inf = NULL;
}

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
    struct MP3GainBatchContext *batch,
    Float_t *dBchange,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
)
{
    int mainloop;
    mpg123_handle *mh = NULL;
    unsigned long ok = !0;
    int mode = 0;
    int crcflag = 0;
    int mpegver = 0;
    unsigned long frame = 0;
    int nchan = 0;
    int bitridx = 0;
    int freqidx = 0;
    long bytesinframe = 0;
    int nprocsamp = 0;
    Float_t maxsample = 0;
    Float_t lsamples[1152];
    Float_t rsamples[1152];
    unsigned char maxgain = 0;
    unsigned char mingain = 0;
    char analysisError = 0;
    long gFilesize = 0;
    int decodeSuccess = 0;

    for (mainloop = fileStart; mainloop < argc; mainloop++) {
#ifdef AACGAIN
        AACGainHandle aacH = aacInfo[mainloop];
#endif
        char *curfilename = argv[mainloop];
        inf = NULL;

        tagInfo[mainloop].recalc |= albumRecalc;

        if (mp3gain_handle_simple_action(
            curfilename,
            tagInfo + mainloop,
            fileTags + mainloop,
            databaseFormat,
            directGain,
            directSingleChannelGain,
            directGainVal,
            batch->gSuccess
#ifdef AACGAIN
            , aacH
#endif
        )) {
            continue;
        }

        if (!databaseFormat) {
            fprintf(stdout, "%s\n", argv[mainloop]);
        }

        if (!mp3gain_open_input_file(argv[mainloop], tagInfo[mainloop].recalc, &gFilesize, &inf, batch->gSuccess
#ifdef AACGAIN
            , aacH
#endif
        )) {
            continue;
        }

        mp3gain_process_file_analysis(
            curfilename, tagInfo + mainloop, fileTags + mainloop, &fileok[mainloop],
            maxAmpOnly, dBGainMod, mp3GainMod, databaseFormat, applyTrack, applyAlbum,
            saveTime, skipTag, autoClip, ignoreClipWarning, batch,
            dBchange, intGainChange, &gFilesize, &inf, &mh, &decodeSuccess, &ok,
            &nprocsamp, lsamples, rsamples, &maxsample, &maxgain, &mingain, &analysisError,
            &frame, &bitridx, &mpegver, &crcflag, &freqidx, &bytesinframe, &mode, &nchan,
            frequency
#ifdef AACGAIN
            , aacH
#endif
        );
    }
}

void mp3gain_free_runtime_state(
    int argc,
    int fileStart,
    struct MP3GainTagInfo *tagInfo,
    int *fileok,
    struct FileTagsStruct *fileTags
#ifdef AACGAIN
    , AACGainHandle *aacInfo
#endif
)
{
    int mainloop;

    free(tagInfo);
    free(fileok);

    for (mainloop = fileStart; mainloop < argc; mainloop++) {
        if (fileTags[mainloop].apeTag) {
            if (fileTags[mainloop].apeTag->otherFields) {
                free(fileTags[mainloop].apeTag->otherFields);
            }
            free(fileTags[mainloop].apeTag);
        }
        if (fileTags[mainloop].lyrics3tag) {
            free(fileTags[mainloop].lyrics3tag);
        }
        if (fileTags[mainloop].id31tag) {
            free(fileTags[mainloop].id31tag);
        }
    }

    free(fileTags);
#ifdef AACGAIN
    free(aacInfo);
#endif
}

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
)
{
    if ((numFiles > 0) && (!applyTrack) && (!analysisTrack)) {
        mp3gain_finish_album_analysis(
            argv, argc, fileStart, fileok, tagInfo, fileTags, albumRecalc,
            maxAmpOnly, dBGainMod, mp3GainMod, databaseFormat, applyAlbum,
            skipTag, saveTime, autoClip, ignoreClipWarning, numFiles, &gSuccess,
            dBchange, intGainChange
#ifdef AACGAIN
            , aacInfo
#endif
        );
    }

    if ((!applyTrack) &&
        (!applyAlbum) &&
        (!directGain) &&
        (!directSingleChannelGain) &&
        (!deleteTag) &&
        (!skipTag) &&
        (!checkTagOnly)) {
        mp3gain_write_dirty_tags(
            argv, fileStart, argc, fileok, tagInfo, fileTags, saveTime
#ifdef AACGAIN
            , aacInfo
#endif
        );
    }

    mp3gain_free_runtime_state(
        argc, fileStart, tagInfo, fileok, fileTags
#ifdef AACGAIN
        , aacInfo
#endif
    );

    return gSuccess ? 0 : 1;
}

#ifdef AACGAIN
unsigned long mp3gain_process_aac_recalc(
    AACGainHandle aacH,
    int maxAmpOnly,
    Float_t *maxsample,
    unsigned char *mingain,
    unsigned char *maxgain,
    struct MP3GainBatchContext *batch,
    char *analysisError,
    char *filename
)
{
    int rc;

    if (*(batch->first)) {
        *(batch->lastfreq) = aac_get_sample_rate(aacH);
        InitGainAnalysis((long)*(batch->lastfreq));
        *analysisError = 0;
        *(batch->first) = 0;
    } else if (aac_get_sample_rate(aacH) != *(batch->lastfreq)) {
        *(batch->lastfreq) = aac_get_sample_rate(aacH);
        ResetSampleFrequency((long)*(batch->lastfreq));
    }

    (*(batch->numFiles))++;

    if (maxAmpOnly) {
        rc = aac_compute_peak(aacH, maxsample, mingain, maxgain,
            g_mp3gain_config.QuietMode ? NULL : reportPercentAnalyzed);
    } else {
        rc = aac_compute_gain(aacH, maxsample, mingain, maxgain,
            g_mp3gain_config.QuietMode ? NULL : reportPercentAnalyzed);
    }

    if (rc != 0) {
        passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 2,
            filename, " is not a valid mp4/m4a file.\n");
        return 0;
    }

    return !0;
}
#endif
