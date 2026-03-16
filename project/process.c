#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "apetag.h"
#include "cli.h"
#include "gain_analysis.h"
#include "mp3gain.h"
#include <mpg123.h>

#include "process.h"

enum {
    MP3GAIN_FULL_RECALC = 1,
    MP3GAIN_AMP_RECALC = 2
};

extern int BadLayer;
extern int LayerSet;
extern int Reckless;
extern int QuietMode;
extern int checkTagOnly;
extern int undoChanges;
extern double lastfreq;
extern int totFiles;
extern unsigned char *wrdpntr;
extern unsigned char buffer[];
extern unsigned char *minGain;
extern unsigned char *maxGain;
extern long inbuffer;
extern unsigned long bitidx;
extern unsigned long filepos;
extern unsigned char *curframe;
extern long arrbytesinframe[16];
extern const double frequency[4][4];
extern FILE *inf;

int getSizeOfFile(char *filename);
unsigned long fillBuffer(long savelastbytes);
unsigned long reportPercentAnalyzed(unsigned long percent, unsigned long bytes);
unsigned long skipID3v2(void);
unsigned long frameSearch(int newfile);
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
void scanFrameGain(void);
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
        exit(1);
    }

    return 1;
}

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
)
{
    *fileok = (int *)malloc(sizeof(int) * argc);
    *tagInfo = (struct MP3GainTagInfo *)calloc(argc, sizeof(struct MP3GainTagInfo));
    *fileTags = (struct FileTagsStruct *)malloc(sizeof(struct FileTagsStruct) * argc);
#ifdef AACGAIN
    *aacInfo = (AACGainHandle *)malloc(sizeof(AACGainHandle) * argc);
#endif

    mp3gain_cli_print_table_header(databaseFormat, checkTagOnly, undoChanges);
    totFiles = argc - fileStart;
    mp3gain_prepare_files_batch(
        argc, argv, fileStart, *fileok, *tagInfo, *fileTags
#ifdef AACGAIN
            , *aacInfo
#endif
    );
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

    albumRecalc = mp3gain_compute_recalc_flags(
        argc, fileStart, tagInfo, applyTrack, analysisTrack, maxAmpOnly
    );

    mp3gain_process_files_batch(
        argc, argv, fileStart, tagInfo, fileTags, fileok, albumRecalc,
        databaseFormat, maxAmpOnly, dBGainMod, mp3GainMod, applyTrack,
        applyAlbum, saveTime, skipTag, autoClip, ignoreClipWarning,
        directSingleChannelGain, directGainVal, directGain, gSuccess,
        first, numFiles, dBchange, intGainChange
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
    BadLayer = 0;
    LayerSet = Reckless;
    *maxgain = 0;
    *mingain = 255;
    inbuffer = 0;
    filepos = 0;
    bitidx = 0;
    return fillBuffer(0);
}

unsigned long mp3gain_prime_mp3_frames(int recalc)
{
    unsigned long ok = !0;

    if (recalc > 0) {
        wrdpntr = buffer;
        ok = skipID3v2();
        ok = frameSearch(!0);
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
        if (!BadLayer) {
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
    LayerSet = 1;
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
    int *first,
    double *lastfreq,
    char *analysisError,
    const double frequency[4][4]
)
{
    if (!maxAmpOnly) {
        if (*first) {
            *lastfreq = frequency[mpegver][freqidx];
            InitGainAnalysis((long)(*lastfreq * 1000.0));
            *analysisError = 0;
            *first = 0;
        } else if (frequency[mpegver][freqidx] != *lastfreq) {
            *lastfreq = frequency[mpegver][freqidx];
            ResetSampleFrequency((long)(*lastfreq * 1000.0));
        }
    } else {
        *analysisError = 0;
    }
}

unsigned long mp3gain_prepare_first_mp3_frame(
    unsigned char *curframe,
    char *filename,
    int *mode,
    int *mpegver,
    int *freqidx,
    int *frame,
    long arrbytesinframe[16]
)
{
    unsigned char *xingcheck;
    int sideinfo_len;
    unsigned long ok = !0;
    bitidx = (curframe[2] >> 4) & 0x0F;

    *mode = (curframe[3] >> 6) & 3;

    if ((curframe[1] & 0x08) == 0x08) {
        sideinfo_len = (*mode == 3) ? 4 + 17 : 4 + 32;
    } else {
        sideinfo_len = (*mode == 3) ? 4 + 9 : 4 + 17;
    }

    if (!(curframe[1] & 0x01)) {
        sideinfo_len += 2;
    }

    xingcheck = curframe + sideinfo_len;
    if ((xingcheck[0] == 'X' && xingcheck[1] == 'i' && xingcheck[2] == 'n' && xingcheck[3] == 'g') ||
        (xingcheck[0] == 'I' && xingcheck[1] == 'n' && xingcheck[2] == 'f' && xingcheck[3] == 'o')) {
        if (bitidx == 0) {
            fprintf(stderr, "%s is free format (not currently supported)\n", filename);
            fflush(stderr);
            ok = 0;
        } else {
            int bytesinframe;

            *mpegver = (curframe[1] >> 3) & 0x03;
            *freqidx = (curframe[2] >> 2) & 0x03;
            bytesinframe = arrbytesinframe[bitidx] + ((curframe[2] >> 1) & 0x01);
            wrdpntr = curframe + bytesinframe;
            ok = frameSearch(0);
        }
    }

    *frame = 1;
    if (ok) {
        *mpegver = (curframe[1] >> 3) & 0x03;
        *freqidx = (curframe[2] >> 2) & 0x03;
    }

    return ok;
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
)
{
    if ((recalc & MP3GAIN_AMP_RECALC) || (recalc & MP3GAIN_FULL_RECALC)) {
        size_t decbytes = 0;
        unsigned char *decout;

        scanFrameGain();
        if (MPG123_OK != mpg123_feed(mh, curframe, bytesinframe)) {
            fprintf(stderr, "Feeding mpg123 failed: %s\n", mpg123_strerror(mh));
            exit(1);
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
                fprintf(stderr, "Unexpected format returned by libmpg123.\n");
                exit(1);
            }
        } else {
#if (MPG123_API_VERSION < 45)
            if (*decodeSuccess == MPG123_NEED_MORE) {
                fprintf(stderr, "Delaying a frame in decoding with old libmpg123.\n");
                decbytes = 0;
            } else {
#endif
                fprintf(stderr, "Failed to decode MPEG frame: %s\n", mpg123_strerror(mh));
                exit(1);
#if (MPG123_API_VERSION < 45)
            }
#endif
        }

        *nprocsamp = decbytes / sizeof(float) / nchan;
        if (*nprocsamp > sizeof(Float_t[1152]) / sizeof(Float_t)) {
            fprintf(stderr, "Too many samples in libmpg123 output.\n");
            exit(1);
        }

        convert_decout((float*)decout, *nprocsamp, nchan, lsamples, rsamples);
        *maxsample = find_maxsample((float*)decout, (*nprocsamp) * nchan, *maxsample);
    } else {
        *decodeSuccess = !MPG123_OK;
        scanFrameGain();
    }

    if (*decodeSuccess == MPG123_OK) {
        if ((!maxAmpOnly) && (recalc & MP3GAIN_FULL_RECALC)) {
            if (AnalyzeSamples(lsamples, rsamples, *nprocsamp, nchan) == GAIN_ANALYSIS_ERROR) {
                fprintf(stderr, "Error analyzing further samples (max time reached)          \n");
                *analysisError = !0;
            }
        }
    }
}

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
)
{
    if (inbuffer >= bytesinframe) {
        maxGain = maxgain;
        minGain = mingain;
        mp3gain_process_frame_audio(
            mh, bytesinframe, nchan, recalc, maxAmpOnly,
            decodeSuccess, nprocsamp, lsamples, rsamples, maxsample, analysisError
        );
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
            mh, bytesinframe, nchan, recalc, maxAmpOnly,
            decodeSuccess, nprocsamp, lsamples, rsamples, maxsample,
            maxgain, mingain, analysisError, frame, gFilesize
        );
#ifdef WIN32
#ifndef __GNUC__
    }
    __except(1) {
        fprintf(stderr, "Error analyzing %s. This mp3 has some very corrupt data.\n", filename);
        fclose(stdout);
        fclose(stderr);
        exit(1);
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
        wrdpntr = curframe + bytesinframe;
        ok = frameSearch(0);
    }

    if (!QuietMode) {
        if (!(frame % 200)) {
            reportPercentAnalyzed(
                (int)(((double)(filepos - (inbuffer - (curframe + bytesinframe - buffer))) * 100.0) / gFilesize),
                gFilesize
            );
        }
    }

    return ok;
}

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
)
{
    *bitridx = (curframe[2] >> 4) & 0x0F;
    if (*bitridx == 0) {
        fprintf(stderr, "%s is free format (not currently supported)\n", filename);
        fflush(stderr);
        return 0;
    }

    *mpegver = (curframe[1] >> 3) & 0x03;
    *crcflag = curframe[1] & 0x01;
    *freqidx = (curframe[2] >> 2) & 0x03;
    *bytesinframe = arrbytesinframe[*bitridx] + ((curframe[2] >> 1) & 0x01);
    *mode = (curframe[3] >> 6) & 0x03;
    *nchan = (*mode == 3) ? 1 : 2;

    return !0;
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
    long arrbytesinframe[16],
    long gFilesize
)
{
    unsigned long ok = !0;

    while (ok) {
        ok = mp3gain_prepare_mp3_frame(
            filename, bitridx, mpegver, crcflag, freqidx,
            bytesinframe, mode, nchan, arrbytesinframe
        );

        if (ok) {
            ok = mp3gain_process_mp3_frame_iteration_safe(
                mh, filename, *bytesinframe, *nchan, recalc, maxAmpOnly,
                decodeSuccess, nprocsamp, lsamples, rsamples, maxsample,
                maxgain, mingain, analysisError, ++(*frame), gFilesize
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
)
{
    if (recalc == 0) {
        return *ok;
    }

#ifdef AACGAIN
    if (aacH) {
        *ok = mp3gain_process_aac_recalc(
            aacH, maxAmpOnly, maxsample, mingain, maxgain,
            first, lastfreq, analysisError, numFiles, filename
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
    if (mp3gain_handle_missing_mp3_frames(*ok, gSuccess, filename
#ifdef AACGAIN
        , aacH
#endif
    )) {
        return *ok;
    }

    mp3gain_mark_valid_file(fileok, numFiles, recalc
#ifdef AACGAIN
        , aacH
#endif
    );

#ifdef AACGAIN
    if (!aacH && (recalc > 0)) {
#else
    if (recalc > 0) {
#endif
        *ok = mp3gain_prepare_first_mp3_frame(curframe, filename, mode, mpegver, freqidx, frame, arrbytesinframe);
        mp3gain_sync_mp3_frequency(*mpegver, *freqidx, maxAmpOnly, first, lastfreq, analysisError, frequency);
        mp3gain_process_mp3_frames(
            mh, filename, recalc, maxAmpOnly,
            decodeSuccess, nprocsamp, lsamples, rsamples, maxsample,
            maxgain, mingain, analysisError, frame, bitridx, mpegver,
            crcflag, freqidx, bytesinframe, mode, nchan, arrbytesinframe, gFilesize
        );
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
    int *first,
    double *lastfreq,
    int *gSuccess,
    int *numFiles,
    Float_t *dBchange,
    int *intGainChange
#ifdef AACGAIN
    , AACGainHandle aacH
#endif
)
{
    if (!QuietMode) {
        fprintf(stderr, "                                                 \r");
    }

    if (!mp3gain_finalize_track_analysis(
        tagInfo, tagInfo->recalc, maxAmpOnly, dBGainMod, mp3GainMod,
        maxsample, mingain, maxgain, dBchange, intGainChange
    )) {
        fprintf(stderr, "Not enough samples in %s to do analysis\n", filename);
        fflush(stderr);
        *gSuccess = 0;
        (*numFiles)--;
        return;
    }

    if (mp3gain_report_track_result(
        filename, databaseFormat, applyTrack, applyAlbum,
        *dBchange, *intGainChange, maxsample, maxgain, mingain
    )) {
        return;
    }

    if (applyTrack) {
        *first = !0;
        mp3gain_apply_track_change(
            filename, tagInfo, fileTags, saveTime, skipTag, autoClip,
            ignoreClipWarning, maxsample, intGainChange, first
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
)
{
#ifdef AACGAIN
    if (!aacH)
#endif
    {
        mp3gain_init_decoder(mh, decodeSuccess);
    }

    if (tagInfo->recalc == 0) {
        mp3gain_load_existing_track_data(tagInfo, maxsample, maxgain, mingain, ok);
    } else {
        mp3gain_prepare_recalc_state(tagInfo, maxsample);
        *ok = mp3gain_run_file_recalc(
            *mh, filename, tagInfo->recalc, maxAmpOnly, decodeSuccess,
            nprocsamp, lsamples, rsamples, maxsample, maxgain, mingain,
            analysisError, first, lastfreq, numFiles, fileok, gSuccess, ok,
            frame, bitridx, mpegver, crcflag, freqidx, bytesinframe, mode,
            nchan, arrbytesinframe, frequency, *gFilesize
#ifdef AACGAIN
            , aacH
#endif
        );
    }

    if (*ok) {
        mp3gain_finish_track_recalc(
            filename, tagInfo, fileTags, maxAmpOnly, dBGainMod, mp3GainMod,
            *maxsample, *mingain, *maxgain, databaseFormat, applyTrack,
            applyAlbum, saveTime, skipTag, autoClip, ignoreClipWarning,
            first, lastfreq, gSuccess, numFiles, dBchange, intGainChange
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
    int *gSuccess,
    int *first,
    int *numFiles,
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
            gSuccess
#ifdef AACGAIN
            , aacH
#endif
        )) {
            continue;
        }

        if (!databaseFormat) {
            fprintf(stdout, "%s\n", argv[mainloop]);
        }

        if (!mp3gain_open_input_file(argv[mainloop], tagInfo[mainloop].recalc, &gFilesize, &inf, gSuccess
#ifdef AACGAIN
            , aacH
#endif
        )) {
            continue;
        }

        mp3gain_process_file_analysis(
            curfilename, tagInfo + mainloop, fileTags + mainloop, &fileok[mainloop],
            maxAmpOnly, dBGainMod, mp3GainMod, databaseFormat, applyTrack, applyAlbum,
            saveTime, skipTag, autoClip, ignoreClipWarning, first, &lastfreq, gSuccess,
            numFiles, dBchange, intGainChange, &gFilesize, &inf, &mh, &decodeSuccess, &ok,
            &nprocsamp, lsamples, rsamples, &maxsample, &maxgain, &mingain, &analysisError,
            &frame, &bitridx, &mpegver, &crcflag, &freqidx, &bytesinframe, &mode, &nchan,
            arrbytesinframe, frequency
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
            skipTag, saveTime, autoClip, ignoreClipWarning, numFiles,
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
    int *first,
    double *lastfreq,
    char *analysisError,
    int *numFiles,
    char *filename
)
{
    int rc;

    if (*first) {
        *lastfreq = aac_get_sample_rate(aacH);
        InitGainAnalysis((long)*lastfreq);
        *analysisError = 0;
        *first = 0;
    } else if (aac_get_sample_rate(aacH) != *lastfreq) {
        *lastfreq = aac_get_sample_rate(aacH);
        ResetSampleFrequency((long)*lastfreq);
    }

    (*numFiles)++;

    if (maxAmpOnly) {
        rc = aac_compute_peak(aacH, maxsample, mingain, maxgain,
            QuietMode ? NULL : reportPercentAnalyzed);
    } else {
        rc = aac_compute_gain(aacH, maxsample, mingain, maxgain,
            QuietMode ? NULL : reportPercentAnalyzed);
    }

    if (rc != 0) {
        passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 2,
            filename, " is not a valid mp4/m4a file.\n");
        exit(1);
    }

    return !0;
}
#endif
