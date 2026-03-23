#ifndef MP3GAIN_CONFIG_H
#define MP3GAIN_CONFIG_H

struct MP3GainGlobalConfig {
    unsigned char maxAmpOnly;
    short int saveTime;
    int whichChannel;
    int BadLayer;
    int LayerSet;
    int Reckless;
    int wrapGain;
    int undoChanges;
    int skipTag;
    int deleteTag;
    int forceRecalculateTag;
    int forceUpdateTag;
    int checkTagOnly;
    int QuietMode;
    int UsingTemp;
    int useId3;
    int writeself;
    int NowWriting;
    double lastfreq;
};

extern struct MP3GainGlobalConfig g_mp3gain_config;

#endif
