/* this file is an extract from d_soundfile.c, from pd master 0.49-0 */

/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* this file contains, first, a collection of soundfile access routines, a
sort of soundfile library.  Second, the "soundfiler" object is defined which
uses the routines to read or write soundfiles, synchronously, from garrays.
These operations are not to be done in "real time" as they may have to wait
for disk accesses (even the write routine.)  Finally, the realtime objects
readsf~ and writesf~ are defined which confine disk operations to a separate
thread so that they can be used in real time.  The readsf~ and writesf~
objects use Posix-like threads.  */

#ifndef _WIN32
#include <unistd.h>
#endif
#include <pthread.h>
#ifdef _WIN32
#include <io.h>
#endif
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <math.h>
#include <limits.h>

#include "m_pd.h"

#define MAXSFCHANS 64

#ifdef _LARGEFILE64_SOURCE
# define open open64
# define lseek lseek64
#define off_t __off64_t
#endif

/* Microsoft Visual Studio does not define these... arg */
#ifdef _MSC_VER
#define off_t long
#define O_CREAT   _O_CREAT
#define O_TRUNC   _O_TRUNC
#define O_WRONLY  _O_WRONLY
#endif

/***************** soundfile header structures ************************/

typedef union _samplelong {
  t_sample f;
  long     l;
} t_sampleuint;

#define FORMAT_WAVE 0
#define FORMAT_AIFF 1
#define FORMAT_NEXT 2

/* the NeXTStep sound header structure; can be big or little endian  */

typedef struct _nextstep
{
    char ns_fileid[4];      /* magic number '.snd' if file is big-endian */
    uint32_t ns_onset;      /* byte offset of first sample */
    uint32_t ns_length;     /* length of sound in bytes */
    uint32_t ns_format;     /* format; see below */
    uint32_t ns_sr;         /* sample rate */
    uint32_t ns_nchans;     /* number of channels */
    char ns_info[4];        /* comment */
} t_nextstep;

#define NS_FORMAT_LINEAR_16     3
#define NS_FORMAT_LINEAR_24     4
#define NS_FORMAT_FLOAT         6
#define SCALE (1./(1024. * 1024. * 1024. * 2.))

/* the WAVE header.  All Wave files are little endian.  We assume
    the "fmt" chunk comes first which is usually the case but perhaps not
    always; same for AIFF and the "COMM" chunk.   */

typedef unsigned word;
typedef unsigned long dword;

typedef struct _wave
{
    char  w_fileid[4];              /* chunk id 'RIFF'            */
    uint32_t w_chunksize;           /* chunk size                 */
    char  w_waveid[4];              /* wave chunk id 'WAVE'       */
    char  w_fmtid[4];               /* format chunk id 'fmt '     */
    uint32_t w_fmtchunksize;        /* format chunk size          */
    uint16_t w_fmttag;              /* format tag (WAV_INT etc)   */
    uint16_t w_nchannels;           /* number of channels         */
    uint32_t w_samplespersec;       /* sample rate in hz          */
    uint32_t w_navgbytespersec;     /* average bytes per second   */
    uint16_t w_nblockalign;         /* number of bytes per frame  */
    uint16_t w_nbitspersample;      /* number of bits in a sample */
    char  w_datachunkid[4];         /* data chunk id 'data'       */
    uint32_t w_datachunksize;       /* length of data chunk       */
} t_wave;

typedef struct _fmt         /* format chunk */
{
    uint16_t f_fmttag;              /* format tag, 1 for PCM      */
    uint16_t f_nchannels;           /* number of channels         */
    uint32_t f_samplespersec;       /* sample rate in hz          */
    uint32_t f_navgbytespersec;     /* average bytes per second   */
    uint16_t f_nblockalign;         /* number of bytes per frame  */
    uint16_t f_nbitspersample;      /* number of bits in a sample */
} t_fmt;

typedef struct _wavechunk           /* ... and the last two items */
{
    char  wc_id[4];                 /* data chunk id, e.g., 'data' or 'fmt ' */
    uint32_t wc_size;               /* length of data chunk       */
} t_wavechunk;

#define WAV_INT 1
#define WAV_FLOAT 3

/* the AIFF header.  I'm assuming AIFC is compatible but don't really know
    that. */

typedef struct _datachunk
{
    char  dc_id[4];                 /* data chunk id 'SSND'       */
    uint32_t dc_size;               /* length of data chunk       */
    uint32_t dc_offset;             /* additional offset in bytes */
    uint32_t dc_block;              /* block size                 */
} t_datachunk;

typedef struct _comm
{
    uint16_t c_nchannels;           /* number of channels         */
    uint16_t c_nframeshi;           /* # of sample frames (hi)    */
    uint16_t c_nframeslo;           /* # of sample frames (lo)    */
    uint16_t c_bitspersamp;         /* bits per sample            */
    unsigned char c_samprate[10];   /* sample rate, 80-bit float! */
} t_comm;

    /* this version is more convenient for writing them out: */
typedef struct _aiff
{
    char  a_fileid[4];              /* chunk id 'FORM'            */
    uint32_t a_chunksize;           /* chunk size                 */
    char  a_aiffid[4];              /* aiff chunk id 'AIFF'       */
    char  a_fmtid[4];               /* format chunk id 'COMM'     */
    uint32_t a_fmtchunksize;        /* format chunk size, 18      */
    uint16_t a_nchannels;           /* number of channels         */
    uint16_t a_nframeshi;           /* # of sample frames (hi)    */
    uint16_t a_nframeslo;           /* # of sample frames (lo)    */
    uint16_t a_bitspersamp;         /* bits per sample            */
    unsigned char a_samprate[10];   /* sample rate, 80-bit float! */
} t_aiff;

#define AIFFHDRSIZE 38      /* probably not what sizeof() gives */
#define AIFFPLUS (AIFFHDRSIZE + 16)  /* header size including SSND chunk hdr */

#define WHDR1 sizeof(t_nextstep)
#define WHDR2 (sizeof(t_wave) > WHDR1 ? sizeof (t_wave) : WHDR1)
#define WRITEHDRSIZE (AIFFPLUS > WHDR2 ? AIFFPLUS : WHDR2)

#define READHDRSIZE (16 > WHDR2 + 2 ? 16 : WHDR2 + 2)

#define OBUFSIZE MAXPDSTRING  /* assume MAXPDSTRING is bigger than headers */

/* this routine returns 1 if the high order byte comes at the lower
address on our architecture (big-endianness.).  It's 1 for Motorola,
0 for Intel: */
/* (from g_array.c) */
static int garray_ambigendian(void)
{
    unsigned short s = 1;
    unsigned char c = *(char *)(&s);
    return (c==0);
}

/* byte swappers */

static uint32_t swap4(uint32_t n, int doit)
{
    if (doit)
        return (((n & 0xff) << 24) | ((n & 0xff00) << 8) |
            ((n & 0xff0000) >> 8) | ((n & 0xff000000) >> 24));
    else return (n);
}

static uint16_t swap2(uint32_t n, int doit)
{
    if (doit)
        return (((n & 0xff) << 8) | ((n & 0xff00) >> 8));
    else return (n);
}

static void swapstring(char *foo, int doit)
{
    if (doit)
    {
        char a = foo[0], b = foo[1], c = foo[2], d = foo[3];
        foo[0] = d; foo[1] = c; foo[2] = b; foo[3] = a;
    }
}

    /* read sample rate from an 80-bit AIFF-compatible number */
static double readaiffsamprate(char * shit, int swap)
{
   unsigned long mantissa, last = 0;
   unsigned char exp;

   swapstring(shit+2, swap);

   mantissa = (unsigned long) *((unsigned long *)(shit+2));
   exp = 30 - *(shit+1);
   while (exp--)
   {
        last = mantissa;
        mantissa >>= 1;
   }
   if (last & 0x00000001) mantissa++;
   return mantissa;
}

    /* write a sample rate as an 80-bit AIFF-compatible number */
static void makeaiffsamprate(double sr, unsigned char *shit)
{
    int exponent;
    double mantissa = frexp(sr, &exponent);
    unsigned long fixmantissa = ldexp(mantissa, 32);
    shit[0] = (exponent+16382)>>8;
    shit[1] = exponent+16382;
    shit[2] = fixmantissa >> 24;
    shit[3] = fixmantissa >> 16;
    shit[4] = fixmantissa >> 8;
    shit[5] = fixmantissa;
    shit[6] = shit[7] = shit[8] = shit[9] = 0;
}

/******************** soundfile access routines **********************/

typedef struct _soundfile_info
{
    int samplerate;
    int channels;
    int bytespersample;
    int headersize;
    int bigendian;
    long bytelimit;
} t_soundfile_info;

static void outlet_soundfile_info(t_outlet *out, t_soundfile_info *info)
{
    t_atom info_list[5];
    SETFLOAT((t_atom *)info_list, (t_float)info->samplerate);
    SETFLOAT((t_atom *)info_list+1, (t_float)(info->headersize < 0 ? 0 : info->headersize));
    SETFLOAT((t_atom *)info_list+2, (t_float)info->channels);
    SETFLOAT((t_atom *)info_list+3, (t_float)info->bytespersample);
    SETSYMBOL((t_atom *)info_list+4, gensym((info->bigendian ? "b" : "l")));
    outlet_list(out, &s_list, 5, (t_atom *)info_list);
}

/* This routine opens a file, looks for either a nextstep or "wave" header,
* seeks to end of it, and fills in bytes per sample and number of channels.
* Only 2- and 3-byte fixed-point samples and 4-byte floating point samples
* are supported.  If "headersize" is nonzero, the
* caller should supply the number of channels, endinanness, and bytes per
* sample; the header is ignored.  Otherwise, the routine tries to read the
* header and fill in the properties.
*/

static int open_soundfile_via_fd(int fd, t_soundfile_info *p_info, long skipframes)
{
    int nchannels, bigendian, bytespersamp, samprate, swap;
    int headersize = 0;
    long sysrtn, bytelimit = 0x7fffffff;
    errno = 0;
    if (p_info->headersize >= 0) /* header detection overridden */
    {
        headersize = p_info->headersize;
        bigendian = p_info->bigendian;
        nchannels = p_info->channels;
        bytespersamp = p_info->bytespersample;
        bytelimit = p_info->bytelimit;
        samprate = p_info->samplerate;
    }
    else
    {
        union
        {
            char b_c[OBUFSIZE];
            t_fmt b_fmt;
            t_nextstep b_nextstep;
            t_wavechunk b_wavechunk;
            t_datachunk b_datachunk;
            t_comm b_commchunk;
        } buf;
        long bytesread = read(fd, buf.b_c, READHDRSIZE);
        int format;
        if (bytesread < 4)
            goto badheader;
        if (!strncmp(buf.b_c, ".snd", 4))
            format = FORMAT_NEXT, bigendian = 1;
        else if (!strncmp(buf.b_c, "dns.", 4))
            format = FORMAT_NEXT, bigendian = 0;
        else if (!strncmp(buf.b_c, "RIFF", 4))
        {
            if (bytesread < 12 || strncmp(buf.b_c + 8, "WAVE", 4))
                goto badheader;
            format = FORMAT_WAVE, bigendian = 0;
        }
        else if (!strncmp(buf.b_c, "FORM", 4))
        {
            if (bytesread < 12 || strncmp(buf.b_c + 8, "AIFF", 4))
                goto badheader;
            format = FORMAT_AIFF, bigendian = 1;
        }
        else
            goto badheader;
        swap = (bigendian != garray_ambigendian());
        if (format == FORMAT_NEXT)   /* nextstep header */
        {
            t_nextstep *nsbuf = &buf.b_nextstep;
            if (bytesread < (int)sizeof(t_nextstep))
                goto badheader;
            nchannels = swap4(nsbuf->ns_nchans, swap);
            format = swap4(nsbuf->ns_format, swap);
            headersize = swap4(nsbuf->ns_onset, swap);
            if (format == NS_FORMAT_LINEAR_16)
                bytespersamp = 2;
            else if (format == NS_FORMAT_LINEAR_24)
                bytespersamp = 3;
            else if (format == NS_FORMAT_FLOAT)
                bytespersamp = 4;
            else goto badheader;
            bytelimit = 0x7fffffff;
            samprate = swap4(nsbuf->ns_sr, swap);
        }
        else if (format == FORMAT_WAVE)     /* wave header */
        {
               t_wavechunk *wavechunk = &buf.b_wavechunk;
               /*  This is awful.  You have to skip over chunks,
               except that if one happens to be a "fmt" chunk, you want to
               find out the format from that one.  The case where the
               "fmt" chunk comes after the audio isn't handled. */
            headersize = 12;
            if (bytesread < 20)
                goto badheader;
                /* First we guess a number of channels, etc., in case there's
                no "fmt" chunk to follow. */
            nchannels = 1;
            bytespersamp = 2;
            samprate = 44100;
                /* copy the first chunk header to beginnning of buffer. */
            memcpy(buf.b_c, buf.b_c + headersize, sizeof(t_wavechunk));
            /* post("chunk %c %c %c %c",
                    ((t_wavechunk *)buf)->wc_id[0],
                    ((t_wavechunk *)buf)->wc_id[1],
                    ((t_wavechunk *)buf)->wc_id[2],
                    ((t_wavechunk *)buf)->wc_id[3]); */
                /* read chunks in loop until we get to the data chunk */
            while (strncmp(wavechunk->wc_id, "data", 4))
            {
                long chunksize = swap4(wavechunk->wc_size,
                    swap), seekto = headersize + chunksize + 8, seekout;
                if (seekto & 1)     /* pad up to even number of bytes */
                    seekto++;
                if (!strncmp(wavechunk->wc_id, "fmt ", 4))
                {
                    long commblockonset = headersize + 8;
                    seekout = (long)lseek(fd, commblockonset, SEEK_SET);
                    if (seekout != commblockonset)
                        goto badheader;
                    if (read(fd, buf.b_c, sizeof(t_fmt)) < (int) sizeof(t_fmt))
                            goto badheader;
                    nchannels = swap2(buf.b_fmt.f_nchannels, swap);
                    format = swap2(buf.b_fmt.f_nbitspersample, swap);
                    if (format == 16)
                        bytespersamp = 2;
                    else if (format == 24)
                        bytespersamp = 3;
                    else if (format == 32)
                        bytespersamp = 4;
                    else goto badheader;
                    samprate = swap4(buf.b_fmt.f_samplespersec, swap);
                }
                seekout = (long)lseek(fd, seekto, SEEK_SET);
                if (seekout != seekto)
                    goto badheader;
                if (read(fd, buf.b_c, sizeof(t_wavechunk)) <
                    (int) sizeof(t_wavechunk))
                        goto badheader;
                /* post("new chunk %c %c %c %c at %d",
                    wavechunk->wc_id[0],
                    wavechunk->wc_id[1],
                    wavechunk->wc_id[2],
                    wavechunk->wc_id[3], seekto); */
                headersize = (int)seekto;
            }
            bytelimit = swap4(wavechunk->wc_size, swap);
            headersize += 8;
        }
        else
        {
                /* AIFF.  same as WAVE; actually predates it.  Disgusting. */
            t_datachunk *datachunk;
            headersize = 12;
            if (bytesread < 20)
                goto badheader;
                /* First we guess a number of channels, etc., in case there's
                no COMM block to follow. */
            nchannels = 1;
            bytespersamp = 2;
            samprate = 44100;
                /* copy the first chunk header to beginnning of buffer. */
            memcpy(buf.b_c, buf.b_c + headersize, sizeof(t_datachunk));
                /* read chunks in loop until we get to the data chunk */
            datachunk = &buf.b_datachunk;
            while (strncmp(datachunk->dc_id, "SSND", 4))
            {
                long chunksize = swap4(datachunk->dc_size,
                    swap), seekto = headersize + chunksize + 8, seekout;
                if (seekto & 1)     /* pad up to even number of bytes */
                    seekto++;
                /* post("chunk %c %c %c %c seek %d",
                    datachunk->dc_id[0],
                    datachunk->dc_id[1],
                    datachunk->dc_id[2],
                    datachunk->dc_id[3], seekto); */
                if (!strncmp(datachunk->dc_id, "COMM", 4))
                {
                    long commblockonset = headersize + 8;
                    t_comm *commchunk;
                    seekout = (long)lseek(fd, commblockonset, SEEK_SET);
                    if (seekout != commblockonset)
                        goto badheader;
                    if (read(fd, buf.b_c, sizeof(t_comm)) <
                        (int) sizeof(t_comm))
                            goto badheader;
                    commchunk = &buf.b_commchunk;
                    nchannels = swap2(commchunk->c_nchannels, swap);
                    format = swap2(commchunk->c_bitspersamp, swap);
                    if (format == 16)
                        bytespersamp = 2;
                    else if (format == 24)
                        bytespersamp = 3;
                    else goto badheader;
                    samprate = readaiffsamprate((char *)commchunk->c_samprate, swap);
                }
                seekout = (long)lseek(fd, seekto, SEEK_SET);
                if (seekout != seekto)
                    goto badheader;
                if (read(fd, buf.b_c, sizeof(t_datachunk)) <
                    (int) sizeof(t_datachunk))
                        goto badheader;
                headersize = (int)seekto;
            }
            bytelimit = swap4(datachunk->dc_size, swap) - 8;
            headersize += sizeof(t_datachunk);
        }
    }
        /* seek past header and any sample frames to skip */
    sysrtn = (long)lseek(fd,
        ((off_t)nchannels) * bytespersamp * skipframes + headersize, 0);
    if (sysrtn != nchannels * bytespersamp * skipframes + headersize)
        return (-1);
     bytelimit -= nchannels * bytespersamp * skipframes;
     if (bytelimit < 0)
        bytelimit = 0;
        /* copy sample format back to caller */
    p_info->headersize = headersize;
    p_info->bigendian = bigendian;
    p_info->channels = nchannels;
    p_info->bytespersample = bytespersamp;
    p_info->bytelimit = bytelimit;
    p_info->samplerate = samprate;
    return (fd);
badheader:
        /* the header wasn't recognized.  We're threadable here so let's not
        print out the error... */
    errno = EIO;
    return (-1);
}

    /* open a soundfile, using open_via_path().  This is used by readsf~ in
    a not-perfectly-threadsafe way.  LATER replace with a thread-hardened
    version of open_soundfile_via_canvas() */
static int open_soundfile(const char *dirname, const char *filename,
    t_soundfile_info *p_info, long skipframes)
{
    char buf[OBUFSIZE], *bufptr;
    int fd, sf_fd;
    fd = open_via_path(dirname, filename, "", buf, &bufptr, MAXPDSTRING, 1);
    if (fd < 0)
        return (-1);
    sf_fd = open_soundfile_via_fd(fd, p_info, skipframes);
    if (sf_fd < 0)
        sys_close(fd);
    return (sf_fd);
}

    /* open a soundfile, using open_via_canvas().  This is used by readsf~ in
    a not-perfectly-threadsafe way.  LATER replace with a thread-hardened
    version of open_soundfile_via_canvas() */
static int open_soundfile_via_canvas(t_canvas *canvas, const char *filename,
    t_soundfile_info *p_info, long skipframes)
{
    char buf[OBUFSIZE], *bufptr;
    int fd, sf_fd;
    fd = canvas_open(canvas, filename, "", buf, &bufptr, MAXPDSTRING, 1);
    if (fd < 0)
        return (-1);
    sf_fd = open_soundfile_via_fd(fd, p_info, skipframes);
    if (sf_fd < 0)
        sys_close(fd);
    return (sf_fd);
}

static void soundfile_xferin_sample(int sfchannels, int nvecs, t_sample **vecs,
    long itemsread, unsigned char *buf, int nitems, int bytespersamp,
    int bigendian)
{
    int i, j;
    unsigned char *sp, *sp2;
    t_sample *fp;
    int nchannels = (sfchannels < nvecs ? sfchannels : nvecs);
    int bytesperframe = bytespersamp * sfchannels;
    for (i = 0, sp = buf; i < nchannels; i++, sp += bytespersamp)
    {
        if (bytespersamp == 2)
        {
            if (bigendian)
            {
                for (j = 0, sp2 = sp, fp=vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, fp++)
                        *fp = SCALE * ((sp2[0] << 24) | (sp2[1] << 16));
            }
            else
            {
                for (j = 0, sp2 = sp, fp=vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, fp++)
                        *fp = SCALE * ((sp2[1] << 24) | (sp2[0] << 16));
            }
        }
        else if (bytespersamp == 3)
        {
            if (bigendian)
            {
                for (j = 0, sp2 = sp, fp=vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, fp++)
                        *fp = SCALE * ((sp2[0] << 24) | (sp2[1] << 16)
                            | (sp2[2] << 8));
            }
            else
            {
                for (j = 0, sp2 = sp, fp=vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, fp++)
                        *fp = SCALE * ((sp2[2] << 24) | (sp2[1] << 16)
                            | (sp2[0] << 8));
            }
        }
        else if (bytespersamp == 4)
        {
            if (bigendian)
            {
                for (j = 0, sp2 = sp, fp=vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, fp++)
                        *(long *)fp = ((sp2[0] << 24) | (sp2[1] << 16)
                            | (sp2[2] << 8) | sp2[3]);
            }
            else
            {
                for (j = 0, sp2 = sp, fp=vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, fp++)
                        *(long *)fp = ((sp2[3] << 24) | (sp2[2] << 16)
                            | (sp2[1] << 8) | sp2[0]);
            }
        }
    }
        /* zero out other outputs */
    for (i = sfchannels; i < nvecs; i++)
        for (j = nitems, fp = vecs[i]; j--; )
            *fp++ = 0;

}

static void soundfile_xferin_words(int sfchannels, int nvecs, t_word **vecs,
    long itemsread, unsigned char *buf, long nitems, int bytespersamp,
    int bigendian)
{
    int i;
    long j;
    unsigned char *sp, *sp2;
    t_word *wp;
    int nchannels = (sfchannels < nvecs ? sfchannels : nvecs);
    int bytesperframe = bytespersamp * sfchannels;
    union
    {
        long long32;
        float float32;
    } word32;
    for (i = 0, sp = buf; i < nchannels; i++, sp += bytespersamp)
    {
        if (bytespersamp == 2)
        {
            if (bigendian)
            {
                for (j = 0, sp2 = sp, wp = vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, wp++)
                        wp->w_float = SCALE * ((sp2[0] << 24) | (sp2[1] << 16));
            }
            else
            {
                for (j = 0, sp2 = sp, wp = vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, wp++)
                        wp->w_float = SCALE * ((sp2[1] << 24) | (sp2[0] << 16));
            }
        }
        else if (bytespersamp == 3)
        {
            if (bigendian)
            {
                for (j = 0, sp2 = sp, wp = vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, wp++)
                        wp->w_float = SCALE * ((sp2[0] << 24) | (sp2[1] << 16)
                            | (sp2[2] << 8));
            }
            else
            {
                for (j = 0, sp2 = sp, wp = vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, wp++)
                        wp->w_float = SCALE * ((sp2[2] << 24) | (sp2[1] << 16)
                            | (sp2[0] << 8));
            }
        }
        else if (bytespersamp == 4)
        {
            if (bigendian)
            {
                for (j = 0, sp2 = sp, wp = vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, wp++)
                {
                    word32.long32 = (sp2[0] << 24) |
                            (sp2[1] << 16) | (sp2[2] << 8) | sp2[3];
                    wp->w_float = word32.float32;
                }
            }
            else
            {
                for (j = 0, sp2 = sp, wp = vecs[i] + itemsread;
                    j < nitems; j++, sp2 += bytesperframe, wp++)
                {
                    word32.long32 = (sp2[3] << 24) |
                        (sp2[2] << 16) | (sp2[1] << 8) | sp2[0];
                    wp->w_float = word32.float32;
                }
            }
        }
    }
        /* zero out other outputs */
    for (i = sfchannels; i < nvecs; i++)
        for (j = nitems, wp = vecs[i]; j--; )
            (wp++)->w_float = 0;

}


