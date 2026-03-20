/*
 *  mp3gain.c - analyzes mp3 files, determines the perceived volume, 
 *      and adjusts the volume of the mp3 accordingly
 *
 *  Copyright (C) 2001-2009 Glen Sawyer
 *  AAC support (C) 2004-2009 David Lasker, Altos Design, Inc.
 *
 *  This library is free software; you can redistribute it and/or
 *  modify it under the terms of the GNU Lesser General Public
 *  License as published by the Free Software Foundation; either
 *  version 2.1 of the License, or (at your option) any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *  Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public
 *  License along with this library; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 *  coding by Glen Sawyer (mp3gain@hotmail.com) 735 W 255 N, Orem, UT 84057-4505 USA
 *    -- go ahead and laugh at me for my lousy coding skillz, I can take it :)
 *       Just do me a favor and let me know how you improve the code.
 *       Thanks.
 *
 *  Unix-ification by Stefan Partheym�ller
 *  (other people have made Unix-compatible alterations-- I just ended up using
 *   Stefan's because it involved the least re-work)
 *
 *  DLL-ification by John Zitterkopf (zitt@hotmail.com)
 *
 *  Additional tweaks by Artur Polaczynski, Mark Armbrust, and others
 */


/*
 *  General warning: I coded this in several stages over the course of several
 *  months. During that time, I changed my mind about my coding style and
 *  naming conventions many, many times. So there's not a lot of consistency
 *  in the code. Sorry about that. I may clean it up some day, but by the time
 *  I would be getting around to it, I'm sure that the more clever programmers
 *  out there will have come up with superior versions already...
 *
 *  So have fun dissecting.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "apetag.h"
#include "cli.h"
#include "id3tag.h"
#include "prep.h"
#include "exec.h"
#include "process.h"
#ifdef AACGAIN
#include "aacgain.h"
#endif

#ifndef WIN32
#undef asWIN32DLL
#ifdef __FreeBSD__
#include <sys/types.h>
#endif /* __FreeBSD__ */
#include <utime.h>
#endif /* WIN32 */

#ifdef WIN32
#include <windows.h>
#include <io.h>
#define SWITCH_CHAR '/'
#else
/* time stamp preservation when using temp file */
# include <sys/stat.h>
# include <utime.h>
# include <errno.h>
# if defined(__BEOS__)
#  include <fs_attr.h>
# endif
#define SWITCH_CHAR '-'
#endif /* WIN32 */

#ifdef __BEOS__
#include <bsd_mem.h>
#endif /* __BEOS__ */

#include <fcntl.h>
#include <string.h>
#include <ctype.h>

#ifndef asWIN32DLL

/* Was in lame.h. */
#include <stdarg.h>

/* Used to be part of decode_i386. */
unsigned char maxAmpOnly;
/* layer3.c */
unsigned char *minGain;
unsigned char *maxGain;

#include <mpg123.h>
#include "gain_analysis.h"
#endif

#include "mp3gain.h"  /*jzitt*/
#include "rg_error.h" /*jzitt*/

#define HEADERSIZE 4

#define CRC16_POLYNOMIAL        0x8005

#define BUFFERSIZE 3000000
#define WRITEBUFFERSIZE 100000

#define FULL_RECALC 1
#define AMP_RECALC 2
#define MIN_MAX_GAIN_RECALC 4

#ifdef AACGAIN
#define AACGAIN_ARG(x)  , x
#else
#define AACGAIN_ARG(x)
#endif

typedef struct {
	unsigned long fileposition;
	unsigned char val[2];
} wbuffer;

unsigned long reportPercentAnalyzed(unsigned long percent, unsigned long bytes);
unsigned long reportPercentWritten(unsigned long percent, unsigned long bytes);
unsigned long skipID3v2(void);
unsigned long frameSearch(int startup);
void scanFrameGain(void);
static void mp3gain_reset_modify_scan_state(void);
static void mp3gain_prime_modify_frame_bits(int crcflag);
static void mp3gain_skip_mpeg1_sideinfo_bits(int mode, int nchan);
static void mp3gain_skip_mpeg2_sideinfo_bits(int mode);
static void mp3gain_set_bit_cursor(unsigned char *ptr, unsigned long bit_offset);
static void mp3gain_advance_bit_cursor(int nbits);
static void mp3gain_skip_to_global_gain_bits(void);
static void mp3gain_skip_past_global_gain_bits(int trailing_bits);
static unsigned char mp3gain_read_current_frame_gain(void);
static void mp3gain_write_current_frame_gain(unsigned char gain);
static void mp3gain_apply_mpeg1_granule_channels(int nchan, int gainchange[2]);
static void mp3gain_apply_mpeg2_channels(int nchan, int gainchange[2]);
static void mp3gain_track_mpeg1_granule_channels(int nchan);
static void mp3gain_track_mpeg2_channels(int nchan);
static void mp3gain_apply_mpeg1_frame(int mode, int nchan, int crcflag, int gainchange[2]);
static void mp3gain_apply_mpeg2_frame(int mode, int nchan, int crcflag, int gainchange[2]);
static unsigned long mp3gain_finish_modify_frame(unsigned long *frame, long bytesinframe, long gFilesize);
static int mp3gain_fail_modify_open(char **outfilename, int error_code, int numStrings, ...);
static void set8Bits(unsigned short val);
static void skipBits(int nbits);
static unsigned char peek8Bits(void);
static void crcWriteHeader(int headerlength, char *header);

/* Yes, yes, I know I should do something about these globals */

wbuffer writebuffer[WRITEBUFFERSIZE];

unsigned long writebuffercnt;

struct MP3ScanState {
    unsigned char buffer[BUFFERSIZE];
    long inbuffer;
    unsigned long bitidx;
    unsigned char *wrdpntr;
    unsigned char *curframe;
    unsigned long filepos;
};
static struct MP3ScanState g_scan_state;

#define buffer (g_scan_state.buffer)
#define inbuffer (g_scan_state.inbuffer)
#define bitidx (g_scan_state.bitidx)
#define wrdpntr (g_scan_state.wrdpntr)
#define curframe (g_scan_state.curframe)
#define filepos (g_scan_state.filepos)

int writeself = 0;
int QuietMode = 0;
int UsingTemp = 1;
int NowWriting = 0;
double lastfreq = -1.0;

int whichChannel = 0;
int BadLayer = 0;
int LayerSet = 0;
int Reckless = 0;
int wrapGain = 0;
int undoChanges = 0;

int skipTag = 0;
int deleteTag = 0;
int forceRecalculateTag = 0;
int forceUpdateTag = 0;
int checkTagOnly = 0;
int useId3 = 0;

int gSuccess;

char *curfilename;

FILE *inf = NULL;

FILE *outf;

short int saveTime;

static const double bitrate[4][16] = {
	{ 1,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, 1 },
	{ 1,  1,  1,  1,  1,  1,  1,  1,   1,   1,   1,   1,   1,   1,   1, 1 },
	{ 1,  8, 16, 24, 32, 40, 48, 56,  64,  80,  96, 112, 128, 144, 160, 1 },
	{ 1, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320, 1 }
};
const double frequency[4][4] = {
	{ 11.025, 12,  8,  1 },
	{      1,  1,  1,  1 },
	{  22.05, 24, 16,  1 },
	{   44.1, 48, 32,  1 }
};

long arrbytesinframe[16];

/* instead of writing each byte change, I buffer them up */
static
void flushWriteBuff() {
	unsigned long i;
	for (i = 0; i < writebuffercnt; i++) {
		fseek(inf,writebuffer[i].fileposition,SEEK_SET);
		fwrite(writebuffer[i].val,1,2,inf);
	}
	writebuffercnt = 0;
};



static
void addWriteBuff(unsigned long pos, unsigned char *vals) {
	if (writebuffercnt >= WRITEBUFFERSIZE) {
		flushWriteBuff();
		fseek(inf,filepos,SEEK_SET);
	}
	writebuffer[writebuffercnt].fileposition = pos;
	writebuffer[writebuffercnt].val[0] = *vals;
	writebuffer[writebuffercnt].val[1] = vals[1];
	writebuffercnt++;
	
};

static int mp3gain_should_stream_temp_output(void)
{
	return UsingTemp && NowWriting;
}

static int mp3gain_write_temp_output_chunk(const unsigned char *data, size_t len)
{
	if (!mp3gain_should_stream_temp_output())
		return 1;

	return fwrite(data, 1, len, outf) == len;
}


/* fill the mp3 buffer */
unsigned long fillBuffer(long savelastbytes) {
	unsigned long i;
	unsigned long skip;
    unsigned long skipbuf;

	skip = 0;
	if (savelastbytes < 0) {
		skip = -savelastbytes;
		savelastbytes = 0;
	}

	if (!mp3gain_write_temp_output_chunk(buffer, (size_t)(inbuffer - savelastbytes))) {
		return 0;
	}

	if (savelastbytes != 0) /* save some of the bytes at the end of the buffer */
		memmove((void*)buffer,(const void*)(buffer+inbuffer-savelastbytes),savelastbytes);
	
	while (skip > 0) { /* skip some bytes from the input file */
        skipbuf = skip > BUFFERSIZE ? BUFFERSIZE : skip;

		i = (unsigned long)fread(buffer,1,skipbuf,inf);
        if (i != skipbuf)
            return 0;

		if (!mp3gain_write_temp_output_chunk(buffer, (size_t)skipbuf)) {
			return 0;
		}
		filepos += i;
        skip -= skipbuf;
	}
	i = (unsigned long)fread(buffer+savelastbytes,1,BUFFERSIZE-savelastbytes,inf);

	filepos = filepos + i;
	inbuffer = i + savelastbytes;
	return i;
}

void mp3gain_reset_mp3_scan_state(void)
{
	inbuffer = 0;
	filepos = 0;
	bitidx = 0;
}

void mp3gain_prime_mp3_buffer_pointer(void)
{
	wrdpntr = buffer;
}

unsigned char *mp3gain_get_current_frame_pointer(void)
{
	return curframe;
}

void mp3gain_set_current_frame_minmax(unsigned char *mingain, unsigned char *maxgain)
{
	minGain = mingain;
	maxGain = maxgain;
}

int mp3gain_can_process_frame(long bytesinframe)
{
	return inbuffer >= bytesinframe;
}

void mp3gain_advance_frame_pointer(long bytesinframe)
{
	wrdpntr = curframe + bytesinframe;
}

unsigned long mp3gain_skip_id3_and_find_first_frame(void)
{
	unsigned long ok;

	ok = skipID3v2();
	if (!ok)
		return ok;

	return frameSearch(!0);
}

unsigned long mp3gain_find_next_mp3_frame(void)
{
	return frameSearch(0);
}

void mp3gain_scan_current_frame_gain(void)
{
	scanFrameGain();
}

static int mp3gain_open_modify_streams(char *filename, char **outfilename)
{
	long outlength;

	if (UsingTemp) {
		fflush(stderr);
		fflush(stdout);
		outlength = (long)strlen(filename);
		*outfilename = (char *)malloc(outlength + 5);
		strcpy(*outfilename, filename);
		if ((filename[outlength-3] == 'T' || filename[outlength-3] == 't') &&
				(filename[outlength-2] == 'M' || filename[outlength-2] == 'm') &&
				(filename[outlength-1] == 'P' || filename[outlength-1] == 'p')) {
			strcat(*outfilename, ".TMP");
		}
		else {
			(*outfilename)[outlength-3] = 'T';
			(*outfilename)[outlength-2] = 'M';
			(*outfilename)[outlength-1] = 'P';
		}

		inf = fopen(filename, "r+b");
		if (inf != NULL) {
			outf = fopen(*outfilename, "wb");
			if (outf == NULL) {
				return mp3gain_fail_modify_open(
					outfilename, M3G_ERR_CANT_MAKE_TMP, 3,
					"\nCan't open ", *outfilename, " for temp writing\n"
				);
			}
		}
	}
	else {
		inf = fopen(filename, "r+b");
	}

	if (inf == NULL) {
		return mp3gain_fail_modify_open(
			outfilename, M3G_ERR_CANT_MODIFY_FILE, 3,
			"\nCan't open ", filename, " for modifying\n"
		);
	}

	return 0;
}

static unsigned long mp3gain_prepare_modify_scan_bootstrap(void)
{
	unsigned long ok;

	mp3gain_reset_modify_scan_state();
	ok = fillBuffer(0);
	if (!ok)
		return ok;

	mp3gain_prime_mp3_buffer_pointer();
	return !0;
}

static unsigned long mp3gain_prepare_modify_first_frame(
	char *filename,
	unsigned long *frame,
	int *mode,
	int *bitridx,
	int *mpegver,
	long *bytesinframe)
{
	unsigned char *xingcheck;
	int sideinfo_len;
	unsigned long ok;

	ok = mp3gain_skip_id3_and_find_first_frame();
	if (!ok)
		return ok;

	LayerSet = 1; /* We've found at least one valid layer 3 frame.
				   * Assume any later layer 1 or 2 frames are just
				   * bitstream corruption
				   */
	*mode = (curframe[3] >> 6) & 3;

	if ((curframe[1] & 0x08) == 0x08)
		sideinfo_len = (*mode == 3) ? 4 + 17 : 4 + 32;
	else
		sideinfo_len = (*mode == 3) ? 4 + 9 : 4 + 17;

	if (!(curframe[1] & 0x01))
		sideinfo_len += 2;

	xingcheck = curframe + sideinfo_len;

	/* LAME CBR files have "Info" tags, not "Xing" tags. */
	if ((xingcheck[0] == 'X' && xingcheck[1] == 'i' && xingcheck[2] == 'n' && xingcheck[3] == 'g') ||
		(xingcheck[0] == 'I' && xingcheck[1] == 'n' && xingcheck[2] == 'f' && xingcheck[3] == 'o')) {
		*bitridx = (curframe[2] >> 4) & 0x0F;
		if (*bitridx == 0) {
			passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 2,
				filename, " is free format (not currently supported)\n");
			return 0;
		}

		*mpegver = (curframe[1] >> 3) & 0x03;
		*bytesinframe = arrbytesinframe[*bitridx] + ((curframe[2] >> 1) & 0x01);
		mp3gain_advance_frame_pointer(*bytesinframe);
		ok = mp3gain_find_next_mp3_frame();
		if (!ok)
			return ok;
	}

	*frame = 1;
	return ok;
}

static unsigned long mp3gain_begin_modify_frames(
	char *filename,
	unsigned long *frame,
	int *mode,
	int *bitridx,
	int *mpegver,
	long *bytesinframe)
{
	unsigned long ok;

	ok = mp3gain_prepare_modify_scan_bootstrap();
	if (!ok)
		return ok;
	ok = mp3gain_prepare_modify_first_frame(
		filename, frame, mode, bitridx, mpegver, bytesinframe
	);
	if (!ok && !BadLayer) {
		passError(MP3GAIN_UNSPECIFED_ERROR, 3,
			"Can't find any valid MP3 frames in file ", filename, "\n");
	}

	return ok;
}

static unsigned long mp3gain_validate_modify_frame(
	char *filename,
	int singlechannel,
	int *bitridx)
{
	*bitridx = (curframe[2] >> 4) & 0x0F;

	if (singlechannel) {
		if ((curframe[3] >> 6) & 0x01) {
			passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 2,
				filename, ": Can't adjust single channel for mono or joint stereo\n");
			return 0;
		}
	}

	if (*bitridx == 0) {
		passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 2,
			filename, " is free format (not currently supported)\n");
		return 0;
	}

	return !0;
}

static unsigned long mp3gain_prepare_modify_current_frame(
	char *filename,
	int singlechannel,
	int *bitridx,
	int *mpegver,
	int *crcflag,
	long *bytesinframe,
	int *mode,
	int *nchan)
{
	unsigned long ok;
	int freqidx;

	ok = mp3gain_validate_modify_frame(filename, singlechannel, bitridx);
	if (!ok)
		return ok;

	ok = mp3gain_prepare_mp3_frame(
		filename, bitridx, mpegver, crcflag, &freqidx, bytesinframe, mode, nchan
	);
	if (!ok)
		return ok;

	mp3gain_prime_modify_frame_bits(*crcflag);
	return !0;
}

static unsigned long mp3gain_process_modify_frame(
	char *filename,
	int singlechannel,
	int gainchange[2],
	unsigned long *frame,
	long gFilesize)
{
	unsigned long ok;
	int bitridx;
	int mpegver;
	int crcflag;
	int mode;
	int nchan;
	long bytesinframe;

	ok = mp3gain_prepare_modify_current_frame(
		filename, singlechannel, &bitridx, &mpegver, &crcflag, &bytesinframe, &mode, &nchan
	);
	if (!ok)
		return ok;

	if (mpegver == 3)
		mp3gain_apply_mpeg1_frame(mode, nchan, crcflag, gainchange);
	else
		mp3gain_apply_mpeg2_frame(mode, nchan, crcflag, gainchange);

	return mp3gain_finish_modify_frame(frame, bytesinframe, gFilesize);
}

static unsigned char mp3gain_apply_channel_gain(unsigned char gain, int gainchange)
{
	if (wrapGain)
		return gain + (unsigned char)gainchange;

	if (gain == 0)
		return gain;

	if ((int)gain + gainchange > 255)
		return 255;
	if ((int)gain + gainchange < 0)
		return 0;

	return gain + (unsigned char)gainchange;
}

static unsigned long mp3gain_current_file_offset(unsigned char *ptr)
{
	return filepos - (inbuffer - (ptr - buffer));
}

static void mp3gain_buffer_write_pair(unsigned char *ptr)
{
	addWriteBuff(mp3gain_current_file_offset(ptr), ptr);
}

static unsigned long mp3gain_modify_progress_percent(long bytesinframe, long gFilesize)
{
	return (unsigned long)(((double)mp3gain_current_file_offset(curframe + bytesinframe) * 100.0) / gFilesize);
}

static unsigned long mp3gain_analyze_progress_percent(long bytesinframe, long gFilesize)
{
	return (unsigned long)(((double)mp3gain_current_file_offset(curframe + bytesinframe) * 100.0) / gFilesize);
}

static void mp3gain_prime_modify_frame_bits(int crcflag)
{
	if (!crcflag)
		mp3gain_set_bit_cursor(curframe + 6, 0);
	else
		mp3gain_set_bit_cursor(curframe + 4, 0);
}

static void mp3gain_prime_mpeg1_global_gain_bits(int mode, int nchan)
{
	mp3gain_set_bit_cursor(wrdpntr + 1, 1);
	mp3gain_skip_mpeg1_sideinfo_bits(mode, nchan);
}

static void mp3gain_prime_mpeg2_global_gain_bits(int mode)
{
	mp3gain_set_bit_cursor(wrdpntr + 1, bitidx);
	mp3gain_skip_mpeg2_sideinfo_bits(mode);
}

static void mp3gain_set_bit_cursor(unsigned char *ptr, unsigned long bit_offset)
{
	wrdpntr = ptr;
	bitidx = bit_offset;
}

static void mp3gain_advance_bit_cursor(int nbits)
{
	skipBits(nbits);
}

static void mp3gain_skip_mpeg1_sideinfo_bits(int mode, int nchan)
{
	if (mode == 3)
		mp3gain_advance_bit_cursor(5);
	else
		mp3gain_advance_bit_cursor(3);

	mp3gain_advance_bit_cursor(nchan * 4);
}

static void mp3gain_skip_mpeg2_sideinfo_bits(int mode)
{
	if (mode == 3)
		mp3gain_advance_bit_cursor(1);
	else
		mp3gain_advance_bit_cursor(2);
}

static void mp3gain_skip_to_global_gain_bits(void)
{
	mp3gain_advance_bit_cursor(21);
}

static void mp3gain_skip_past_global_gain_bits(int trailing_bits)
{
	mp3gain_advance_bit_cursor(trailing_bits);
}

static unsigned char mp3gain_read_current_frame_gain(void)
{
	return peek8Bits();
}

static void mp3gain_write_current_frame_gain(unsigned char gain)
{
	set8Bits(gain);
}

static void mp3gain_prepare_scan_frame_bits(int mpegver, int mode, int nchan, int crcflag)
{
	mp3gain_prime_modify_frame_bits(crcflag);

	if (mpegver == 3)
		mp3gain_prime_mpeg1_global_gain_bits(mode, nchan);
	else
		mp3gain_prime_mpeg2_global_gain_bits(mode);
}

static void mp3gain_apply_frame_channel_gain(int gainchange, int trailing_bits)
{
	unsigned char gain;

	mp3gain_skip_to_global_gain_bits();
	gain = mp3gain_read_current_frame_gain();
	gain = mp3gain_apply_channel_gain(gain, gainchange);
	mp3gain_write_current_frame_gain(gain);
	mp3gain_skip_past_global_gain_bits(trailing_bits);
}

static void mp3gain_track_frame_channel_gain(int trailing_bits)
{
	int gain;

	mp3gain_skip_to_global_gain_bits();
	gain = mp3gain_read_current_frame_gain();
	if (*minGain > gain) {
		*minGain = gain;
	}
	if (*maxGain < gain) {
		*maxGain = gain;
	}
	mp3gain_skip_past_global_gain_bits(trailing_bits);
}

static void mp3gain_apply_mpeg1_granule_channels(int nchan, int gainchange[2])
{
	int ch;

	for (ch = 0; ch < nchan; ch++) {
		mp3gain_apply_frame_channel_gain(gainchange[ch], 38);
	}
}

static void mp3gain_apply_mpeg2_channels(int nchan, int gainchange[2])
{
	int ch;

	for (ch = 0; ch < nchan; ch++) {
		mp3gain_apply_frame_channel_gain(gainchange[ch], 42);
	}
}

static void mp3gain_track_mpeg1_granule_channels(int nchan)
{
	int ch;

	for (ch = 0; ch < nchan; ch++) {
		mp3gain_track_frame_channel_gain(38);
	}
}

static void mp3gain_track_mpeg2_channels(int nchan)
{
	int ch;

	for (ch = 0; ch < nchan; ch++) {
		mp3gain_track_frame_channel_gain(42);
	}
}

static void mp3gain_finalize_modified_frame_header(int nchan, int crcflag, int mono_header_bits, int stereo_header_bits)
{
	if (!crcflag) {
		if (nchan == 1)
			crcWriteHeader(mono_header_bits, (char*)curframe);
		else
			crcWriteHeader(stereo_header_bits, (char*)curframe);
		if (!UsingTemp)
			mp3gain_buffer_write_pair(curframe + 4);
	}
}

static unsigned long mp3gain_advance_modify_frame_pointer(long bytesinframe)
{
	wrdpntr = curframe + bytesinframe;
	return frameSearch(0);
}

static void mp3gain_apply_mpeg1_frame(int mode, int nchan, int crcflag, int gainchange[2])
{
	int gr;

	mp3gain_prime_mpeg1_global_gain_bits(mode, nchan);
	for (gr = 0; gr < 2; gr++) {
		mp3gain_apply_mpeg1_granule_channels(nchan, gainchange);
	}

	mp3gain_finalize_modified_frame_header(nchan, crcflag, 23, 38);
}

static void mp3gain_apply_mpeg2_frame(int mode, int nchan, int crcflag, int gainchange[2])
{
	mp3gain_prime_mpeg2_global_gain_bits(mode);
	mp3gain_apply_mpeg2_channels(nchan, gainchange);

	mp3gain_finalize_modified_frame_header(nchan, crcflag, 15, 23);
}

static unsigned long mp3gain_finish_modify_frame(
	unsigned long *frame,
	long bytesinframe,
	long gFilesize)
{
	unsigned long ok = !0;

	if (!QuietMode) {
		(*frame)++;
		if ((*frame) % 200 == 0) {
			ok = reportPercentWritten(mp3gain_modify_progress_percent(bytesinframe, gFilesize), gFilesize);
			if (!ok)
				return ok;
		}
	}

	return mp3gain_advance_modify_frame_pointer(bytesinframe);
}

static void mp3gain_reset_modify_scan_state(void)
{
	writebuffercnt = 0;
	inbuffer = 0;
	filepos = 0;
	bitidx = 0;
}

static void mp3gain_finish_modify_reporting(long gFilesize)
{
	if (!QuietMode) {
#ifndef asWIN32DLL
		fprintf(stderr, "                                                   \r");
#else
		sendpercentdone(100, gFilesize);
#endif
	}
	fflush(stderr);
	fflush(stdout);
}

static int mp3gain_finish_modify_result(int result)
{
	NowWriting = 0;
	return result;
}

static void mp3gain_close_modify_streams(void)
{
	if (outf != NULL) {
		fclose(outf);
		outf = NULL;
	}
	if (inf != NULL) {
		fclose(inf);
		inf = NULL;
	}
}

static int mp3gain_fail_modify_open(char **outfilename, int error_code, int numStrings, ...)
{
	va_list marker;

	if (UsingTemp && (outf != NULL)) {
		fclose(outf);
		outf = NULL;
	}
	if (inf != NULL) {
		fclose(inf);
		inf = NULL;
	}

	va_start(marker, numStrings);
	{
		char *errstr;
		size_t totalStrLen = 0;
		int i;
		va_list copy;

		va_copy(copy, marker);
		for (i = 0; i < numStrings; i++) {
			totalStrLen += strlen(va_arg(copy, const char *));
		}
		va_end(copy);

		errstr = (char *)malloc(totalStrLen + 1);
		errstr[0] = '\0';
		for (i = 0; i < numStrings; i++) {
			strcat(errstr, va_arg(marker, const char *));
		}
		DoError(errstr, MP3GAIN_UNSPECIFED_ERROR);
		free(errstr);
	}
	va_end(marker);

	free(*outfilename);
	*outfilename = NULL;
	NowWriting = 0;
	return error_code;
}

static int mp3gain_finalize_temp_modify(char *filename, char *outfilename)
{
	long outlength, inlength;

	while (fillBuffer(0))
		;
	fflush(outf);
#ifdef WIN32
	outlength = _filelength(_fileno(outf));
	inlength = _filelength(_fileno(inf));
#else
	fseek(outf, 0, SEEK_END);
	fseek(inf, 0, SEEK_END);
	outlength = ftell(outf);
	inlength = ftell(inf);
#endif
#ifdef __BEOS__
	/* some stuff to preserve attributes */
	do {
		DIR *attrs = NULL;
		struct dirent *de;
		struct attr_info ai;
		int infd, outfd;
		void *attrdata;

		infd = fileno(inf);
		if (infd < 0)
			goto attrerror;
		outfd = fileno(outf);
		if (outfd < 0)
			goto attrerror;
		attrs = fs_fopen_attr_dir(infd);
		while ((de = fs_read_attr_dir(attrs)) != NULL) {
			if (fs_stat_attr(infd, de->d_name, &ai) < B_OK)
				goto attrerror;
			if ((attrdata = malloc(ai.size)) == NULL)
				goto attrerror;
			fs_read_attr(infd, de->d_name, ai.type, 0, attrdata, ai.size);
			fs_write_attr(outfd, de->d_name, ai.type, 0, attrdata, ai.size);
			free(attrdata);
		}
		fs_close_attr_dir(attrs);
		break;
attrerror:
		if (attrdata)
			free(attrdata);
		if (attrs)
			fs_close_attr_dir(attrs);
		fprintf(stderr, "can't preserve attributes for '%s': %s\n", filename, strerror(errno));
	} while (0);
#endif
	mp3gain_close_modify_streams();

	if (outlength != inlength) {
		deleteFile(outfilename);
		passError(MP3GAIN_UNSPECIFED_ERROR, 3,
			"Not enough temp space on disk to modify ", filename,
			"\nEither free some space, or switch off \"temp file\" option with -T\n");
		return M3G_ERR_NOT_ENOUGH_TMP_SPACE;
	}

	if (deleteFile(filename)) {
		deleteFile(outfilename);
		passError(MP3GAIN_UNSPECIFED_ERROR, 3,
			"Can't open ", filename, " for modifying\n");
		return M3G_ERR_CANT_MODIFY_FILE;
	}

	if (moveFile(outfilename, filename)) {
		passError(MP3GAIN_UNSPECIFED_ERROR, 9,
			"Problem re-naming ", outfilename, " to ", filename,
			"\nThe mp3 was correctly modified, but you will need to re-name ",
			outfilename, " to ", filename,
			" yourself.\n");
		return M3G_ERR_RENAME_TMP;
	}

	if (saveTime)
		fileTime(filename, setStoredTime);

	return 0;
}

static void mp3gain_finalize_inplace_modify(char *filename)
{
	flushWriteBuff();
	mp3gain_close_modify_streams();
	if (saveTime)
		fileTime(filename, setStoredTime);
}

#ifdef asWIN32DLL
static void mp3gain_cancel_modify(char *filename, char *outfilename)
{
	if (inf != NULL) {
		fclose(inf);
		inf = NULL;
	}
	if (UsingTemp) {
		if (outf != NULL) {
			fclose(outf);
			outf = NULL;
		}
		deleteFile(outfilename);
		free(outfilename);
		passError(MP3GAIN_CANCELLED, 2, "Cancelled processing of ", filename);
	}
	else {
		passError(MP3GAIN_CANCELLED, 3, "Cancelled processing.\n", filename, " is probably corrupted now.");
	}
	if (saveTime)
		fileTime(filename, setStoredTime);
}
#endif

unsigned long mp3gain_report_frame_progress(unsigned long frame, long bytesinframe, long gFilesize)
{
	if (!QuietMode) {
		if (!(frame % 200)) {
			return reportPercentAnalyzed(
				(int)mp3gain_analyze_progress_percent(bytesinframe, gFilesize),
				gFilesize
			);
		}
	}

	return !0;
}


static const unsigned char maskLeft8bits[8] = {
	0x00, 0x80, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC, 0xFE };

static const unsigned char maskRight8bits[8] = {
	0xFF, 0x7F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01 };

static
void set8Bits(unsigned short val) {

	val <<= (8 - bitidx);
	wrdpntr[0] &= maskLeft8bits[bitidx];
	wrdpntr[0] |= (val  >> 8);
	wrdpntr[1] &= maskRight8bits[bitidx];
	wrdpntr[1] |= (val  & 0xFF);
	
	if (!UsingTemp) 
		mp3gain_buffer_write_pair(wrdpntr);
}



static
void skipBits(int nbits) {

	bitidx += nbits;
	wrdpntr += (bitidx >> 3);
	bitidx &= 7;

	return;
}



static
unsigned char peek8Bits() {
	unsigned short rval;

	rval = wrdpntr[0];
	rval <<= 8;
	rval |= wrdpntr[1];
	rval >>= (8 - bitidx);

	return (rval & 0xFF);

}



unsigned long skipID3v2() {
/*
 *  An ID3v2 tag can be detected with the following pattern:
 *    $49 44 33 yy yy xx zz zz zz zz
 *  Where yy is less than $FF, xx is the 'flags' byte and zz is less than
 *  $80.
 */
	unsigned long ok;
	unsigned long ID3Size;

	ok = 1;

	if (wrdpntr[0] == 'I' && wrdpntr[1] == 'D' && wrdpntr[2] == '3' 
		&& wrdpntr[3] < 0xFF && wrdpntr[4] < 0xFF) {

		ID3Size = (long)(wrdpntr[9]) | ((long)(wrdpntr[8]) << 7) |
			((long)(wrdpntr[7]) << 14) | ((long)(wrdpntr[6]) << 21);

		ID3Size += 10;

		wrdpntr = wrdpntr + ID3Size;

		if ((wrdpntr+HEADERSIZE-buffer) > inbuffer) {
			ok = fillBuffer(inbuffer-(wrdpntr-buffer));
			wrdpntr = buffer;
		}
	}

	return ok;
}

void passError(MMRESULT lerrnum, int numStrings, ...)
{
    char * errstr;
    size_t totalStrLen = 0;
    int i;
    va_list marker;

    va_start(marker, numStrings);
    for (i = 0; i < numStrings; i++) {
        totalStrLen += strlen(va_arg(marker, const char *));
    }
    va_end(marker);

    errstr = (char *)malloc(totalStrLen + 3);
    errstr[0] = '\0';

    va_start(marker, numStrings);
    for (i = 0; i < numStrings; i++) {
        strcat(errstr,va_arg(marker, const char *));
    }
    va_end(marker);

    DoError(errstr,lerrnum);
    free(errstr);
    errstr = NULL;
}

unsigned long frameSearch(int startup) {
	unsigned long ok;
	int done;
    static int startfreq;
    static int startmpegver;
	long tempmpegver;
	double bitbase;
	int i;

	done = 0;
	ok = 1;

	if ((wrdpntr+HEADERSIZE-buffer) > inbuffer) {
		ok = fillBuffer(inbuffer-(wrdpntr-buffer));
		wrdpntr = buffer;
		if (!ok) done = 1;
	}

	while (!done) {
		
		done = 1;

		if ((wrdpntr[0] & 0xFF) != 0xFF)
			done = 0;       /* first 8 bits must be '1' */
		else if ((wrdpntr[1] & 0xE0) != 0xE0)
			done = 0;       /* next 3 bits are also '1' */
		else if ((wrdpntr[1] & 0x18) == 0x08)
			done = 0;       /* invalid MPEG version */
		else if ((wrdpntr[2] & 0xF0) == 0xF0)
			done = 0;       /* bad bitrate */
		else if ((wrdpntr[2] & 0xF0) == 0x00)
			done = 0;       /* we'll just completely ignore "free format" bitrates */
		else if ((wrdpntr[2] & 0x0C) == 0x0C)
			done = 0;       /* bad sample frequency */
		else if ((wrdpntr[1] & 0x06) != 0x02) { /* not Layer III */
			if (!LayerSet) {
				switch (wrdpntr[1] & 0x06) {
					case 0x06:
						BadLayer = !0;
						passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 2,
                            curfilename, " is an MPEG Layer I file, not a layer III file\n");
						return 0;
					case 0x04:
						BadLayer = !0;
						passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 2,
                            curfilename, " is an MPEG Layer II file, not a layer III file\n");
						return 0;
				}
			}
			done = 0; /* probably just corrupt data, keep trying */
		}
        else if (startup) {
            startmpegver = wrdpntr[1] & 0x18;
            startfreq = wrdpntr[2] & 0x0C;
			tempmpegver = startmpegver >> 3;
			if (tempmpegver == 3)
				bitbase = 1152.0;
			else
				bitbase = 576.0;

			for (i = 0; i < 16; i++)
				arrbytesinframe[i] = (long)(floor(floor((bitbase*bitrate[tempmpegver][i])/frequency[tempmpegver][startfreq >> 2]) / 8.0));

        }
        else { /* !startup -- if MPEG version or frequency is different, 
                              then probably not correctly synched yet */
            if ((wrdpntr[1] & 0x18) != startmpegver)
                done = 0;
            else if ((wrdpntr[2] & 0x0C) != startfreq)
                done = 0;
            else if ((wrdpntr[2] & 0xF0) == 0) /* bitrate is "free format" probably just 
                                                  corrupt data if we've already found 
                                                  valid frames */
                done = 0;
        }

		if (!done) wrdpntr++;

		if ((wrdpntr+HEADERSIZE-buffer) > inbuffer) {
			ok = fillBuffer(inbuffer-(wrdpntr-buffer));
			wrdpntr = buffer;
			if (!ok) done = 1;
		}
	}

	if (ok) {
		if (inbuffer - (wrdpntr-buffer) < (arrbytesinframe[(wrdpntr[2] >> 4) & 0x0F] + ((wrdpntr[2] >> 1) & 0x01))) {
			ok = fillBuffer(inbuffer-(wrdpntr-buffer));
			wrdpntr = buffer;
		}
		bitidx = 0;
		curframe = wrdpntr;
	}
	return ok;
}

unsigned long mp3gain_prepare_first_mp3_frame(
	char *filename,
	int *mode,
	int *mpegver,
	int *freqidx,
	unsigned long *frame)
{
	unsigned char *xingcheck;
	int sideinfo_len;
	unsigned long ok = !0;

	bitidx = (curframe[2] >> 4) & 0x0F;
	*mode = (curframe[3] >> 6) & 3;

	if ((curframe[1] & 0x08) == 0x08)
		sideinfo_len = (*mode == 3) ? 4 + 17 : 4 + 32;
	else
		sideinfo_len = (*mode == 3) ? 4 + 9 : 4 + 17;

	if (!(curframe[1] & 0x01))
		sideinfo_len += 2;

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
			mp3gain_advance_frame_pointer(bytesinframe);
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

unsigned long mp3gain_prepare_mp3_frame(
	char *filename,
	int *bitridx,
	int *mpegver,
	int *crcflag,
	int *freqidx,
	long *bytesinframe,
	int *mode,
	int *nchan)
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



static
int crcUpdate(int value, int crc)
{
    int i;
    value <<= 8;
    for (i = 0; i < 8; i++) {
		value <<= 1;
		crc <<= 1;

		if (((crc ^ value) & 0x10000))
			crc ^= CRC16_POLYNOMIAL;
	}
    return crc;
}



static
void crcWriteHeader(int headerlength, char *header)
{
    int crc = 0xffff; /* (jo) init crc16 for error_protection */
    int i;

    crc = crcUpdate(((unsigned char*)header)[2], crc);
    crc = crcUpdate(((unsigned char*)header)[3], crc);
    for (i = 6; i < headerlength; i++) {
	crc = crcUpdate(((unsigned char*)header)[i], crc);
    }

    header[4] = crc >> 8;
    header[5] = crc & 255;
}


long getSizeOfFile(char *filename)
{
    long size = 0;
    FILE *file;

    file = fopen(filename, "rb");
    if (file) {    
        fseek(file, 0, SEEK_END);
        size = ftell(file);
        fclose(file);
    }
  
    return size;
}


int deleteFile(char *filename)
{
    return remove(filename);
}

int moveFile(char *currentfilename, char *newfilename)
{
    return rename(currentfilename, newfilename);
}



	/* Get File size and datetime stamp */




void fileTime(char *filename, timeAction action)
{
	static        int  timeSaved=0;
#ifdef WIN32
	HANDLE outfh;
	static FILETIME create_time, access_time, write_time;
#else
    static struct stat savedAttributes;
#endif

    if (action == storeTime) {
#ifdef WIN32
		outfh = CreateFile((LPCTSTR)filename,
							GENERIC_READ,
							FILE_SHARE_READ,
							NULL,
							OPEN_EXISTING,
							FILE_ATTRIBUTE_NORMAL,
							NULL);
		if (outfh != INVALID_HANDLE_VALUE) {
			if (GetFileTime(outfh,&create_time,&access_time,&write_time))
				timeSaved = !0;

			CloseHandle(outfh);
		}
#else
        timeSaved = (stat(filename, &savedAttributes) == 0);
#endif
    }
    else {
        if (timeSaved) {
#ifdef WIN32
			outfh = CreateFile((LPCTSTR)filename,
						GENERIC_WRITE,
						0,
						NULL,
						OPEN_EXISTING,
						FILE_ATTRIBUTE_NORMAL,
						NULL);
			if (outfh != INVALID_HANDLE_VALUE) {
				SetFileTime(outfh,&create_time,&access_time,&write_time);
				CloseHandle(outfh);
			}
#else
			struct utimbuf setTime;	
			
			setTime.actime = savedAttributes.st_atime;
			setTime.modtime = savedAttributes.st_mtime;
			timeSaved = 0;

			utime(filename, &setTime);
#endif
		}
    }      
}

unsigned long reportPercentWritten(unsigned long percent, unsigned long bytes)
{
    int ok = 1;

#ifndef asWIN32DLL
    fprintf(stderr,"                                                \r %2lu%% of %lu bytes written\r"
        ,percent,bytes);
    fflush(stderr);
#else
    /* report % back to calling app */
    ok = sendpercentdone( (int)percent, bytes ); 
    //non-zero return means error bail out
    if ( ok != 0)
	    return 0;
    ok = 1; /* allow us to continue processing file */
#endif

    return ok;
}

int numFiles, totFiles;
unsigned long reportPercentAnalyzed(unsigned long percent, unsigned long bytes)
{
	char fileDivFiles[21];
    fileDivFiles[0]='\0';

    if (totFiles-1)	/* if 1 file then don't show [x/n] */
	    sprintf(fileDivFiles,"[%d/%d]",numFiles,totFiles);

	fprintf(stderr,"                                           \r%s %2lu%% of %lu bytes analyzed\r"
		,fileDivFiles,percent,bytes);
	fflush(stderr);
    return 1;
}

void scanFrameGain() {
	int crcflag;
	int mpegver;
	int mode;
	int nchan;
	int gr;

	mpegver = (curframe[1] >> 3) & 0x03;
	crcflag = curframe[1] & 0x01;
	mode = (curframe[3] >> 6) & 0x03;
	nchan = (mode == 3) ? 1 : 2;

	mp3gain_prepare_scan_frame_bits(mpegver, mode, nchan, crcflag);

	if (mpegver == 3) { /* 9 bit main_data_begin */
		for (gr = 0; gr < 2; gr++) {
			mp3gain_track_mpeg1_granule_channels(nchan);
		}
	} else { /* mpegver != 3 */
		/* only one granule, so no loop */
		mp3gain_track_mpeg2_channels(nchan);
	}
}

int changeGain(char *filename AACGAIN_ARG(AACGainHandle aacH), int leftgainchange, int rightgainchange) {
  unsigned long ok;
  int modifyResult;
  int openResult;
  int mode;
  unsigned long frame;
  int bitridx;
  long bytesinframe;
  int mpegver;
  long gFilesize = 0;
  char *outfilename;
  int gainchange[2];
  int singlechannel;

  outfilename = NULL;
  frame = 0;
  BadLayer = 0;
  LayerSet = Reckless;

  NowWriting = !0;

  if ((leftgainchange == 0) && (rightgainchange == 0))
	  return 0;

#ifdef AACGAIN
  if (aacH)
  {
      int rc = aac_modify_gain(aacH, leftgainchange, rightgainchange, 
          QuietMode ? NULL : reportPercentWritten);
      if (rc)
          passError(MP3GAIN_FILEFORMAT_NOTSUPPORTED, 1, "failed to modify gain\n");
      return mp3gain_finish_modify_result(rc);
  }
#endif

  gainchange[0] = leftgainchange;
  gainchange[1] = rightgainchange;
  singlechannel = !(leftgainchange == rightgainchange);
	  
  if (saveTime)
    fileTime(filename, storeTime);
  
  gFilesize = getSizeOfFile(filename);

  openResult = mp3gain_open_modify_streams(filename, &outfilename);
  if (openResult != 0) {
	  return openResult;
  }
  else {
	ok = mp3gain_begin_modify_frames(
		filename, &frame, &mode, &bitridx, &mpegver, &bytesinframe
	);
	if (ok) {
		
#ifdef asWIN32DLL
		while (ok && (!blnCancel)) {
#else
		while (ok) {
#endif
			ok = mp3gain_process_modify_frame(
				filename, singlechannel, gainchange, &frame, gFilesize
			);
		}
	}

#ifdef asWIN32DLL
	if (blnCancel) { //need to clean up as best as possible
		mp3gain_cancel_modify(filename, outfilename);
		NowWriting = 0;
		return;
	}
#endif

	mp3gain_finish_modify_reporting(gFilesize);
	if (UsingTemp) {
		modifyResult = mp3gain_finalize_temp_modify(filename, outfilename);
		free(outfilename);
		if (modifyResult != 0) {
			return mp3gain_finish_modify_result(modifyResult);
		}
	}
	else {
		mp3gain_finalize_inplace_modify(filename);
	}
  }

  return mp3gain_finish_modify_result(0);
}


#ifndef asWIN32DLL

#ifdef AACGAIN
void WriteAacGainTags (AACGainHandle aacH, struct MP3GainTagInfo *info) {
    if (info->haveAlbumGain)
        aac_set_tag_float(aacH, replaygain_album_gain, info->albumGain);
    if (info->haveAlbumPeak)
        aac_set_tag_float(aacH, replaygain_album_peak, info->albumPeak);
    if (info->haveAlbumMinMaxGain)
        aac_set_tag_int_2(aacH, replaygain_album_minmax, info->albumMinGain, info->albumMaxGain);
    if (info->haveTrackGain)
        aac_set_tag_float(aacH, replaygain_track_gain, info->trackGain);
    if (info->haveTrackPeak)
        aac_set_tag_float(aacH, replaygain_track_peak, info->trackPeak);
    if (info->haveMinMaxGain)
        aac_set_tag_int_2(aacH, replaygain_track_minmax, info->minGain, info->maxGain);
    if (info->haveUndo)
        aac_set_tag_int_2(aacH, replaygain_undo, info->undoLeft, info->undoRight);
}
#endif



void WriteMP3GainTag(char *filename AACGAIN_ARG(AACGainHandle aacH), struct MP3GainTagInfo *info, struct FileTagsStruct *fileTags, int saveTimeStamp)
{
#ifdef AACGAIN
	if (aacH) {
		WriteAacGainTags(aacH, info);
	} else
#endif
	if (useId3) {
		/* Write ID3 tag; remove stale APE tag if it exists. */
		if (WriteMP3GainID3Tag(filename, info, saveTimeStamp) >= 0)
			RemoveMP3GainAPETag(filename, saveTimeStamp);
	} else {
		/* Write APE tag */
		WriteMP3GainAPETag(filename, info, fileTags, saveTimeStamp);
	}
}


void changeGainAndTag(char *filename AACGAIN_ARG(AACGainHandle aacH), int leftgainchange, int rightgainchange, struct MP3GainTagInfo *tag, struct FileTagsStruct *fileTag) {
	double dblGainChange;
	int curMin;
	int curMax;

	if (leftgainchange != 0 || rightgainchange != 0) {
		if (!changeGain(filename AACGAIN_ARG(aacH), leftgainchange, rightgainchange)) {
			if (!tag->haveUndo) {
				tag->undoLeft = 0;
				tag->undoRight = 0;
			}
			tag->dirty = !0;
			tag->undoRight -= rightgainchange;
			tag->undoLeft -= leftgainchange;
			tag->undoWrap = wrapGain;

			/* if undo == 0, then remove Undo tag */
			tag->haveUndo = !0;
	/* on second thought, don't remove it. Shortening the tag causes full file copy, which is slow so we avoid it if we can
			tag->haveUndo = 
				((tag->undoRight != 0) || 
				 (tag->undoLeft != 0));
	*/

			if (leftgainchange == rightgainchange) { /* don't screw around with other fields if mis-matched left/right */
				dblGainChange = leftgainchange * 1.505; /* approx. 5 * log10(2) */
				if (tag->haveTrackGain) {
					tag->trackGain -= dblGainChange;
				}
				if (tag->haveTrackPeak) {
					tag->trackPeak *= pow(2.0,(double)(leftgainchange)/4.0);
				}
				if (tag->haveAlbumGain) {
					tag->albumGain -= dblGainChange;
				}
				if (tag->haveAlbumPeak) {
					tag->albumPeak *= pow(2.0,(double)(leftgainchange)/4.0);
				}
				if (tag->haveMinMaxGain) {
					curMin = tag->minGain;
					curMax = tag->maxGain;
					curMin += leftgainchange;
					curMax += leftgainchange;
					if (wrapGain) {
						if (curMin < 0 || curMin > 255 || curMax < 0 || curMax > 255) {
							/* we've lost the "real" min or max because of wrapping */
							tag->haveMinMaxGain = 0;
						}
					} else {
						tag->minGain = tag->minGain == 0 ? 0 : curMin < 0 ? 0 : curMin > 255 ? 255 : curMin;
						tag->maxGain = curMax < 0 ? 0 : curMax > 255 ? 255 : curMax;
					}
				}
				if (tag->haveAlbumMinMaxGain) {
					curMin = tag->albumMinGain;
					curMax = tag->albumMaxGain;
					curMin += leftgainchange;
					curMax += leftgainchange;
					if (wrapGain) {
						if (curMin < 0 || curMin > 255 || curMax < 0 || curMax > 255) {
							/* we've lost the "real" min or max because of wrapping */
							tag->haveAlbumMinMaxGain = 0;
						}
					} else {
						tag->albumMinGain = tag->albumMinGain == 0 ? 0 : curMin < 0 ? 0 : curMin > 255 ? 255 : curMin;
						tag->albumMaxGain = curMax < 0 ? 0 : curMax > 255 ? 255 : curMax;
					}
				}
			} // if (leftgainchange == rightgainchange ...
			WriteMP3GainTag(filename AACGAIN_ARG(aacH), tag, fileTag, saveTime);
		} // if (!changeGain(filename ...
	}// if (leftgainchange !=0 ...

}


int queryUserForClipping(char * argv_mainloop,int intGainChange)
{
	int ch;

	fprintf(stderr,"\nWARNING: %s may clip with mp3 gain change %d\n",argv_mainloop,intGainChange);
	ch = 0;
	fflush(stdout);
	fflush(stderr);
	while ((ch != 'Y') && (ch != 'N')) {
		fprintf(stderr,"Make change? [y/n]:");
		fflush(stderr);
		ch = getchar();
		if (ch == EOF) {
			ch='N';
		}
		ch = toupper(ch);
	}
	if (ch == 'N')
		return 0;

	return 1;
}

void showVersion(char *progname) {
#ifdef AACGAIN
	fprintf(stderr,"aacgain version %s, derived from mp3gain version %s\n",AACGAIN_VERSION,MP3GAIN_VERSION);
#else
	fprintf(stderr,"%s version %s\n",progname,MP3GAIN_VERSION);
#endif
}


void wrapExplanation() {
	fprintf(stderr,"Here's the problem:\n");
	fprintf(stderr,"The \"global gain\" field that mp3gain adjusts is an 8-bit unsigned integer, so\n");
    fprintf(stderr,"the possible values are 0 to 255.\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"MOST mp3 files (in fact, ALL the mp3 files I've examined so far) don't go\n");
    fprintf(stderr,"over 230. So there's plenty of headroom on top-- you can increase the gain\n");
    fprintf(stderr,"by 37dB (multiplying the amplitude by 76) without a problem.\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"The problem is at the bottom of the range. Some encoders create frames with\n");
    fprintf(stderr,"0 as the global gain for silent frames.\n");
    fprintf(stderr,"What happens when you _lower_ the global gain by 1?\n");
    fprintf(stderr,"Well, in the past, mp3gain always simply wrapped the result up to 255.\n");
    fprintf(stderr,"That way, if you lowered the gain by any amount and then raised it by the\n");
    fprintf(stderr,"same amount, the mp3 would always be _exactly_ the same.\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"There are a few encoders out there, unfortunately, that create 0-gain frames\n");
    fprintf(stderr,"with other audio data in the frame.\n");
    fprintf(stderr,"As long as the global gain is 0, you'll never hear the data.\n");
    fprintf(stderr,"But if you lower the gain on such a file, the global gain is suddenly _huge_.\n");
    fprintf(stderr,"If you play this modified file, there might be a brief, very loud blip.\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"So now the default behavior of mp3gain is to _not_ wrap gain changes.\n");
    fprintf(stderr,"In other words,\n");
    fprintf(stderr,"1) If the gain change would make a frame's global gain drop below 0,\n");
    fprintf(stderr,"   then the global gain is set to 0.\n");
    fprintf(stderr,"2) If the gain change would make a frame's global gain grow above 255,\n");
    fprintf(stderr,"   then the global gain is set to 255.\n");
    fprintf(stderr,"3) If a frame's global gain field is already 0, it is not changed, even if\n");
    fprintf(stderr,"   the gain change is a positive number\n");
    fprintf(stderr,"\n");
    fprintf(stderr,"To use the original \"wrapping\" behavior, use the \"%cw\" switch.\n",SWITCH_CHAR);
#ifdef AACGAIN
    fprintf(stderr,"\n");
    fprintf(stderr,"The \"%cw\" switch is not supported for AAC files. An attempt to wrap\n",SWITCH_CHAR);
    fprintf(stderr,"an AAC file is treated as an error, and the file will not be modified.\n");
#endif
    exit(0);

}



void errUsage(char *progname) {
	showVersion(progname);
	fprintf(stderr,"copyright(c) 2001-2009 by Glen Sawyer\n");
#ifdef AACGAIN
	fprintf(stderr,"AAC support copyright(c) 2004-2009 David Lasker, Altos Design, Inc.\n");
#endif
	fprintf(stderr,"uses mpglib, which can be found at http://www.mpg123.de\n");
#ifdef AACGAIN
    fprintf(stderr,"AAC support uses faad2 (http://www.audiocoding.com), and\n");
    fprintf(stderr,"mpeg4ip's mp4v2 (http://www.mpeg4ip.net)\n");
#endif
	fprintf(stderr,"Usage: %s [options] <infile> [<infile 2> ...]\n",progname);
	fprintf(stderr,"  --use %c? or %ch for a full list of options\n",SWITCH_CHAR,SWITCH_CHAR);
    fclose(stdout);
    fclose(stderr);
	exit(1);
}



void fullUsage(char *progname) {
		showVersion(progname);
		fprintf(stderr,"copyright(c) 2001-2009 by Glen Sawyer\n");
#ifdef AACGAIN
	    fprintf(stderr,"AAC support copyright(c) 2004-2009 David Lasker, Altos Design, Inc.\n");
#endif
		fprintf(stderr,"uses mpglib, which can be found at http://www.mpg123.de\n");
#ifdef AACGAIN
        fprintf(stderr,"AAC support uses faad2 (http://www.audiocoding.com), and\n");
        fprintf(stderr,"mpeg4ip's mp4v2 (http://www.mpeg4ip.net)\n");
#endif
		fprintf(stderr,"Usage: %s [options] <infile> [<infile 2> ...]\n",progname);
		fprintf(stderr,"options:\n");
		fprintf(stderr,"\t%cv - show version number\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cg <i>  - apply gain i without doing any analysis\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cl 0 <i> - apply gain i to channel 0 (left channel)\n",SWITCH_CHAR);
		fprintf(stderr,"\t          without doing any analysis (ONLY works for STEREO files,\n");
		fprintf(stderr,"\t          not Joint Stereo)\n");
		fprintf(stderr,"\t%cl 1 <i> - apply gain i to channel 1 (right channel)\n",SWITCH_CHAR);
		fprintf(stderr,"\t%ce - skip Album analysis, even if multiple files listed\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cr - apply Track gain automatically (all files set to equal loudness)\n",SWITCH_CHAR);
		fprintf(stderr,"\t%ck - automatically lower Track/Album gain to not clip audio\n",SWITCH_CHAR);
		fprintf(stderr,"\t%ca - apply Album gain automatically (files are all from the same\n",SWITCH_CHAR);
		fprintf(stderr,"\t              album: a single gain change is applied to all files, so\n");
		fprintf(stderr,"\t              their loudness relative to each other remains unchanged,\n");
		fprintf(stderr,"\t              but the average album loudness is normalized)\n");
		fprintf(stderr,"\t%cm <i> - modify suggested MP3 gain by integer i\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cd <n> - modify suggested dB gain by floating-point n\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cc - ignore clipping warning when applying gain\n",SWITCH_CHAR);
		fprintf(stderr,"\t%co - output is a database-friendly tab-delimited list\n",SWITCH_CHAR);
		fprintf(stderr,"\t%ct - mp3gain writes modified mp3 to temp file, then deletes original \n",SWITCH_CHAR);
		fprintf(stderr,"\t     instead of modifying bytes in original file (default)\n");
		fprintf(stderr,"\t%cT - mp3gain directly modifies mp3 file (opposite of %ct)\n",SWITCH_CHAR,SWITCH_CHAR);
#ifdef AACGAIN
		fprintf(stderr,"\t     Ignored for AAC files.\n");
#endif
		fprintf(stderr,"\t%cq - Quiet mode: no status messages\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cp - Preserve original file timestamp\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cx - Only find max. amplitude of file\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cf - Assume input file is an MPEG 2 Layer III file\n",SWITCH_CHAR);
		fprintf(stderr,"\t     (i.e. don't check for mis-named Layer I or Layer II files)\n");
#ifdef AACGAIN
		fprintf(stderr,"\t      This option is ignored for AAC files.\n");
#endif
		fprintf(stderr,"\t%c? or %ch - show this message\n",SWITCH_CHAR,SWITCH_CHAR);
		fprintf(stderr,"\t%cs c - only check stored tag info (no other processing)\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cs d - delete stored tag info (no other processing)\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cs s - skip (ignore) stored tag info (do not read or write tags)\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cs r - force re-calculation (do not read tag info)\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cs i - use ID3v2 tag for MP3 gain info\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cs a - use APE tag for MP3 gain info (default)\n",SWITCH_CHAR);
		fprintf(stderr,"\t%cu - undo changes made (based on stored tag info)\n",SWITCH_CHAR);
        fprintf(stderr,"\t%cw - \"wrap\" gain change if gain+change > 255 or gain+change < 0\n",SWITCH_CHAR);
#ifdef AACGAIN
        fprintf(stderr,"\t      MP3 only. (use \"%c? wrap\" switch for a complete explanation)\n",SWITCH_CHAR);
#else
        fprintf(stderr,"\t      (use \"%c? wrap\" switch for a complete explanation)\n",SWITCH_CHAR);
#endif
		fprintf(stderr,"If you specify %cr and %ca, only the second one will work\n",SWITCH_CHAR,SWITCH_CHAR);
		fprintf(stderr,"If you do not specify %cc, the program will stop and ask before\n     applying gain change to a file that might clip\n",SWITCH_CHAR);
        fclose(stdout);
        fclose(stderr);
		exit(0);
}

#ifdef AACGAIN
void ReadAacTags(AACGainHandle gh, struct MP3GainTagInfo *info)
{
    int p1, p2;

    if (aac_get_tag_float(gh, replaygain_album_gain, &info->albumGain) == 0)
        info->haveAlbumGain = !0;
    if (aac_get_tag_float(gh, replaygain_album_peak, &info->albumPeak) == 0)
        info->haveAlbumPeak = !0;
    if (aac_get_tag_int_2(gh, replaygain_album_minmax, &p1, &p2) == 0)
    {
        info->albumMinGain = p1;
        info->albumMaxGain = p2;
        info->haveAlbumMinMaxGain = !0;
    }
    if (aac_get_tag_float(gh, replaygain_track_gain, &info->trackGain) == 0)
        info->haveTrackGain = !0;
    if (aac_get_tag_float(gh, replaygain_track_peak, &info->trackPeak) == 0)
        info->haveTrackPeak = !0;
    if (aac_get_tag_int_2(gh, replaygain_track_minmax, &p1, &p2) == 0)
    {
        info->minGain = p1;
        info->maxGain = p2;
        info->haveMinMaxGain = !0;
    }
    if (aac_get_tag_int_2(gh, replaygain_undo, &p1, &p2) == 0)
    {
        info->undoLeft = p1;
        info->undoRight = p2;
        info->haveUndo = !0;
    }
}
#endif

void dumpTaginfo(struct MP3GainTagInfo *info) {
  fprintf(stderr, "haveAlbumGain       %d  albumGain %f\n",info->haveAlbumGain, info->albumGain);
  fprintf(stderr, "haveAlbumPeak       %d  albumPeak %f\n",info->haveAlbumPeak, info->albumPeak);
  fprintf(stderr, "haveAlbumMinMaxGain %d  min %d  max %d\n",info->haveAlbumMinMaxGain, info->albumMinGain, info->albumMaxGain);
  fprintf(stderr, "haveTrackGain       %d  trackGain %f\n",info->haveTrackGain, info->trackGain);
  fprintf(stderr, "haveTrackPeak       %d  trackPeak %f\n",info->haveTrackPeak, info->trackPeak);
  fprintf(stderr, "haveMinMaxGain      %d  min %d  max %d\n",info->haveMinMaxGain, info->minGain, info->maxGain);
}

void convert_decout(float *decout, int nprocsamp, int nchan, Float_t *lsamples, Float_t *rsamples)
{
	int n;
	if(nchan == 1)
		for(n=0; n<nprocsamp; ++n)
			*lsamples++ = 32768.0* *decout++;
	else if(nchan == 2)
		for(n=0; n<nprocsamp; ++n)
		{
			*lsamples++ = 32767.0 * *decout++;
			*rsamples++ = 32767.0 * *decout++;
		}
}

float find_maxsample(float *decout, int nsamples, float maxsample)
{
	int n;
	for(n=0; n<nsamples; ++n)
	{
		float val = *decout++ * 32768.0;
		if(val < 0)
			val = -val;
		if(val > maxsample)
			maxsample = val;
	}
	return maxsample;
}

#ifdef WIN32
int __cdecl main(int argc, char **argv) { /*make sure this one is standard C declaration*/
#else
int main(int argc, char **argv) {
#endif
	double dBchange;
	int intGainChange = 0;
	int first = 1;
	int ignoreClipWarning = 0;
	int autoClip = 0;
	int applyTrack = 0;
	int applyAlbum = 0;
	int analysisTrack = 0;
	int fileStart;
	int databaseFormat = 0;
	int *fileok;
	int directGain = 0;
	int directSingleChannelGain = 0;
	int directGainVal = 0;
	int mp3GainMod = 0;
	double dBGainMod = 0;
    struct MP3GainTagInfo *tagInfo;
	struct FileTagsStruct *fileTags;
	struct MP3GainCliOptions cli_options;

#ifdef AACGAIN
    AACGainHandle *aacInfo;
#endif

	mpg123_init();
    gSuccess = 1;
	mp3gain_cli_init(&cli_options);
	mp3gain_cli_parse(argc, argv, &cli_options);
	mp3gain_cli_apply_options(&cli_options, &fileStart, &ignoreClipWarning, &autoClip, &applyTrack, &applyAlbum, &analysisTrack, &databaseFormat, &directGain, &directSingleChannelGain, &directGainVal, &mp3GainMod, &dBGainMod);
	numFiles = 0;

	/* now stored in tagInfo---  maxsample = malloc(sizeof(Float_t) * argc); */
    /* now stored in tagInfo---  maxgain = malloc(sizeof(unsigned char) * argc); */
    /* now stored in tagInfo---  mingain = malloc(sizeof(unsigned char) * argc); */
	if (!mp3gain_prepare_runtime_batch(argc, argv, fileStart, databaseFormat, &fileok, &tagInfo, &fileTags AACGAIN_ARG(aacInfo)))
		return 1;
	return mp3gain_run_batch(argv, argc, fileStart, tagInfo, fileTags, fileok, maxAmpOnly, dBGainMod, mp3GainMod, applyTrack, analysisTrack, applyAlbum, databaseFormat, saveTime, skipTag, autoClip, ignoreClipWarning, &directGain, directSingleChannelGain, directGainVal, &gSuccess, &first, &numFiles, &dBchange, &intGainChange, deleteTag, checkTagOnly AACGAIN_ARG(aacInfo));
}
#endif /* asWIN32DLL */
















