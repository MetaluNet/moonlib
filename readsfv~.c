#ifndef _WIN32
/* Copyright (c) 1997-1999 Miller Puckette.
* For information on usage and redistribution, and for a DISCLAIMER OF ALL
* WARRANTIES, see the file, "LICENSE.txt," in this distribution.  */

/* this file contains, first, a collection of soundfile access routines, a
sort of soundfile library.  Second, the "soundfiler" object is defined which
uses the routines to read or write soundfiles, synchronously, from garrays.
These operations are not to be done in "real time" as they may have to wait
for disk accesses (even the write routine.)  Finally, the realtime objects
readsf~ and writesf~ are defined which confine disk operations to a separate
thread so that they can be used in real time.  The real-time disk access
objects are available for linux only so far, although they could be compiled
for Windows if someone were willing to find a Pthreads package for it. */

/* this is a version of readsf~ that works with a (positive) speed parameter.

	Antoine Rousseau
*/

#include "m_pd.h"

#define MAXSFCHANS 8

#include "_soundfile.c"

static void interpolate(int nvec,float **invec,int nin,
                        float **outvec,int nout)
{
    float r=nin/(float)nout;
    int i,j;

    for(i=0; i<nout; i++)
        for(j=0; j<nvec; j++)
            outvec[j][i]=invec[j][(int)(i*r)];
}

/************************* readsf object ******************************/

/* READSF uses the Posix threads package; for the moment we're Linux
only although this should be portable to the other platforms.

Each instance of readsf~ owns a "child" thread for doing the UNIX (NT?) file
reading.  The parent thread signals the child each time:
    (1) a file wants opening or closing;
    (2) we've eaten another 1/16 of the shared buffer (so that the
    	child thread should check if it's time to read some more.)
The child signals the parent whenever a read has completed.  Signalling
is done by setting "conditions" and putting data in mutex-controlled common
areas.
*/

//#ifdef __linux__

#define MAXBYTESPERSAMPLE 4
#define MAXVECSIZE 128

#define READSIZE 65536
#define WRITESIZE 65536
#define DEFBUFPERCHAN 262144
#define MINBUFSIZE (4 * READSIZE)
#define MAXBUFSIZE 16777216 	/* arbitrary; just don't want to hang malloc */

#define REQUEST_NOTHING 0
#define REQUEST_OPEN 1
#define REQUEST_CLOSE 2
#define REQUEST_QUIT 3
#define REQUEST_BUSY 4

#define STATE_IDLE 0
#define STATE_STARTUP 1
#define STATE_STREAM 2

static t_class *readsfv_class;

static t_sample *(tmpvec[MAXSFCHANS]);

typedef struct _readsf
{
    t_object x_obj;
    t_canvas *x_canvas;
    t_clock *x_clock;
    char *x_buf;    	    	    	    /* soundfile buffer */
    int x_bufsize;  	    	    	    /* buffer size in bytes */
    int x_noutlets; 	    	    	    /* number of audio outlets */
    t_sample *(x_outvec[MAXSFCHANS]);	    /* audio vectors */
    int x_vecsize;  	    	    	    /* vector size for transfers */
    t_outlet *x_bangout;  	    	    /* bang-on-done outlet */
    int x_state;    	    	    	    /* opened, running, or idle */
    /* parameters to communicate with subthread */
    int x_requestcode;	    /* pending request from parent to I/O thread */
    const char *x_filename;	    /* file to open (string is permanently allocated) */
    int x_fileerror;	    /* slot for "errno" return */
    int x_skipheaderbytes;  /* size of header we'll skip */
    int x_bytespersample;   /* bytes per sample (2 or 3) */
    int x_bigendian;        /* true if file is big-endian */
    int x_sfchannels;	    /* number of channels in soundfile */
    long x_onsetframes;	    /* number of sample frames to skip */
    long x_bytelimit;	    /* max number of data bytes to read */
    int x_fd;	    	    /* filedesc */
    int x_fifosize; 	    /* buffer size appropriately rounded down */
    int x_fifohead; 	    /* index of next byte to get from file */
    int x_fifotail; 	    /* index of next byte the ugen will read */
    int x_eof;   	    /* true if fifohead has stopped changing */
    int x_sigcountdown;     /* counter for signalling child for more data */
    int x_sigperiod;	    /* number of ticks per signal */
    int x_filetype; 	    /* writesf~ only; type of file to create */
    int x_itemswritten;     /* writesf~ only; items writen */
    int x_swap; 	    /* writesf~ only; true if byte swapping */
    float x_f; 	    	    /* writesf~ only; scalar for signal inlet */
    /*----HACK------*/
    float x_speed;    /*speed of reading*/
    float x_frac;    /*fractionnal part of sample to play next buffer*/

    pthread_mutex_t x_mutex;
    pthread_cond_t x_requestcondition;
    pthread_cond_t x_answercondition;
    pthread_t x_childthread;
} t_readsf;


/************** the child thread which performs file I/O ***********/

#if 0
static void pute(char *s)   /* debug routine */
{
    write(2, s, strlen(s));
}
#else
#define pute(x)
#endif

#if 1
#define sfread_cond_wait pthread_cond_wait
#define sfread_cond_signal pthread_cond_signal
#else
#include <sys/time.h>    /* debugging version... */
#include <sys/types.h>
static void readsf_fakewait(pthread_mutex_t *b)
{
    struct timeval timout;
    timout.tv_sec = 0;
    timout.tv_usec = 1000000;
    pthread_mutex_unlock(b);
    select(0, 0, 0, 0, &timout);
    pthread_mutex_lock(b);
}

static void readsf_banana( void)
{
    struct timeval timout;
    timout.tv_sec = 0;
    timout.tv_usec = 200000;
    pute("banana1\n");
    select(0, 0, 0, 0, &timout);
    pute("banana2\n");
}


#define sfread_cond_wait(a,b) readsf_fakewait(b)
#define sfread_cond_signal(a)
#endif

static void *readsf_child_main(void *zz)
{
    t_readsf *x = zz;
    pute("1\n");
    pthread_mutex_lock(&x->x_mutex);
    while (1)
    {
        int fd, fifohead;
        char *buf;
        pute("0\n");
        if (x->x_requestcode == REQUEST_NOTHING)
        {
            pute("wait 2\n");
            sfread_cond_signal(&x->x_answercondition);
            sfread_cond_wait(&x->x_requestcondition, &x->x_mutex);
            pute("3\n");
        }
        else if (x->x_requestcode == REQUEST_OPEN)
        {
            char boo[80];
            int sysrtn, wantbytes;

            /* copy file stuff out of the data structure so we can
            relinquish the mutex while we're in open_soundfile(). */
			t_soundfile_info info;

			info.samplerate = 0;
			info.channels = x->x_sfchannels;
			info.bytespersample = x->x_bytespersample;
			info.headersize = x->x_skipheaderbytes;
			info.bigendian = x->x_bigendian;
			info.bytelimit = 0x7fffffff;

            long onsetframes = x->x_onsetframes;
            const char *filename = x->x_filename;
            const char *dirname = canvas_getdir(x->x_canvas)->s_name;
            /* alter the request code so that an ensuing "open" will get
            noticed. */
            pute("4\n");
            x->x_requestcode = REQUEST_BUSY;
            x->x_fileerror = 0;

            /* if there's already a file open, close it */
            if (x->x_fd >= 0)
            {
                fd = x->x_fd;
                pthread_mutex_unlock(&x->x_mutex);
                close (fd);
                pthread_mutex_lock(&x->x_mutex);
                x->x_fd = -1;
                if (x->x_requestcode != REQUEST_BUSY)
                    goto lost;
            }
            /* open the soundfile with the mutex unlocked */
            pthread_mutex_unlock(&x->x_mutex);
            fd = open_soundfile(dirname, filename, &info, onsetframes);
            pthread_mutex_lock(&x->x_mutex);

            pute("5\n");
            /* copy back into the instance structure. */
            x->x_bytespersample = info.bytespersample;
            x->x_sfchannels = info.channels;
            x->x_bigendian = info.bigendian;
            x->x_fd = fd;
            x->x_bytelimit = info.bytelimit;
            if (fd < 0)
            {
                x->x_fileerror = errno;
                x->x_eof = 1;
                pute("open failed\n");
                pute(filename);
                pute(dirname);
                goto lost;
            }
            /* check if another request has been made; if so, field it */
            if (x->x_requestcode != REQUEST_BUSY)
                goto lost;
            pute("6\n");
            x->x_fifohead = 0;
            /* set fifosize from bufsize.  fifosize must be a
            multiple of the number of bytes eaten for each DSP
            tick.  We pessimistically assume MAXVECSIZE samples
            per tick since that could change.  There could be a
            problem here if the vector size increases while a
            soundfile is being played...  */
            x->x_fifosize = x->x_bufsize - (x->x_bufsize %
                                            (x->x_bytespersample * x->x_sfchannels * MAXVECSIZE));
            /* arrange for the "request" condition to be signalled 16
            times per buffer */
            sprintf(boo, "fifosize %d\n",
                    x->x_fifosize);
            pute(boo);
            x->x_sigcountdown = x->x_sigperiod =
                                    (x->x_fifosize /
                                     (16 * x->x_bytespersample * x->x_sfchannels *
                                      x->x_vecsize));
            /* in a loop, wait for the fifo to get hungry and feed it */

            while (x->x_requestcode == REQUEST_BUSY)
            {
                int fifosize = x->x_fifosize;
                pute("77\n");
                if (x->x_eof)
                    break;
                if (x->x_fifohead >= x->x_fifotail)
                {
                    /* if the head is >= the tail, we can immediately read
                    to the end of the fifo.  Unless, that is, we would
                    read all the way to the end of the buffer and the
                    "tail" is zero; this would fill the buffer completely
                    which isn't allowed because you can't tell a completely
                    full buffer from an empty one. */
                    if (x->x_fifotail || (fifosize - x->x_fifohead > READSIZE))
                    {
                        wantbytes = fifosize - x->x_fifohead;
                        if (wantbytes > READSIZE)
                            wantbytes = READSIZE;
                        if (wantbytes > x->x_bytelimit)
                            wantbytes = x->x_bytelimit;
                        sprintf(boo, "head %d, tail %d, size %d\n",
                                x->x_fifohead, x->x_fifotail, wantbytes);
                        pute(boo);
                    }
                    else
                    {
                        pute("wait 7a ...\n");
                        sfread_cond_signal(&x->x_answercondition);
                        pute("signalled\n");
                        sfread_cond_wait(&x->x_requestcondition,
                                         &x->x_mutex);
                        pute("7a done\n");
                        continue;
                    }
                }
                else
                {
                    /* otherwise check if there are at least READSIZE
                    bytes to read.  If not, wait and loop back. */
                    wantbytes =  x->x_fifotail - x->x_fifohead - 1;
                    if (wantbytes < READSIZE)
                    {
                        pute("wait 7...\n");
                        sfread_cond_signal(&x->x_answercondition);
                        sfread_cond_wait(&x->x_requestcondition,
                                         &x->x_mutex);
                        pute("7 done\n");
                        continue;
                    }
                    else wantbytes = READSIZE;
                }
                pute("8\n");
                fd = x->x_fd;
                buf = x->x_buf;
                fifohead = x->x_fifohead;
                pthread_mutex_unlock(&x->x_mutex);
                sysrtn = read(fd, buf + fifohead, wantbytes);
                pthread_mutex_lock(&x->x_mutex);
                if (x->x_requestcode != REQUEST_BUSY)
                    break;
                if (sysrtn < 0)
                {
                    pute("fileerror\n");
                    x->x_fileerror = errno;
                    break;
                }
                else if (sysrtn == 0)
                {
                    x->x_eof = 1;
                    break;
                }
                else
                {
                    x->x_fifohead += sysrtn;
                    x->x_bytelimit -= sysrtn;
                    if (x->x_bytelimit <= 0)
                    {
                        x->x_eof = 1;
                        break;
                    }
                    if (x->x_fifohead == fifosize)
                        x->x_fifohead = 0;
                }
                sprintf(boo, "after: head %d, tail %d\n",
                        x->x_fifohead, x->x_fifotail);
                pute(boo);
                /* signal parent in case it's waiting for data */
                sfread_cond_signal(&x->x_answercondition);
            }
lost:

            if (x->x_requestcode == REQUEST_BUSY)
                x->x_requestcode = REQUEST_NOTHING;
            /* fell out of read loop: close file if necessary,
            set EOF and signal once more */
            if (x->x_fd >= 0)
            {
                fd = x->x_fd;
                pthread_mutex_unlock(&x->x_mutex);
                close (fd);
                pthread_mutex_lock(&x->x_mutex);
                x->x_fd = -1;
            }
            sfread_cond_signal(&x->x_answercondition);

        }
        else if (x->x_requestcode == REQUEST_CLOSE)
        {
            if (x->x_fd >= 0)
            {
                fd = x->x_fd;
                pthread_mutex_unlock(&x->x_mutex);
                close (fd);
                pthread_mutex_lock(&x->x_mutex);
                x->x_fd = -1;
            }
            if (x->x_requestcode == REQUEST_CLOSE)
                x->x_requestcode = REQUEST_NOTHING;
            sfread_cond_signal(&x->x_answercondition);
        }
        else if (x->x_requestcode == REQUEST_QUIT)
        {
            if (x->x_fd >= 0)
            {
                fd = x->x_fd;
                pthread_mutex_unlock(&x->x_mutex);
                close (fd);
                pthread_mutex_lock(&x->x_mutex);
                x->x_fd = -1;
            }
            x->x_requestcode = REQUEST_NOTHING;
            sfread_cond_signal(&x->x_answercondition);
            break;
        }
        else
        {
            pute("13\n");
        }
    }
    pute("thread exit\n");
    pthread_mutex_unlock(&x->x_mutex);
    return (0);
}

/******** the object proper runs in the calling (parent) thread ****/

static void readsf_tick(t_readsf *x);

static void *readsf_new(t_floatarg fnchannels, t_floatarg fbufsize)
{
    t_readsf *x;
    int nchannels = fnchannels, bufsize = fbufsize, i;
    char *buf;

    if (nchannels < 1)
        nchannels = 1;
    else if (nchannels > MAXSFCHANS)
        nchannels = MAXSFCHANS;
    if (bufsize <= 0) bufsize = DEFBUFPERCHAN * nchannels;
    else if (bufsize < MINBUFSIZE)
        bufsize = MINBUFSIZE;
    else if (bufsize > MAXBUFSIZE)
        bufsize = MAXBUFSIZE;
    buf = getbytes(bufsize);
    if (!buf) return (0);

    x = (t_readsf *)pd_new(readsfv_class);

    for (i = 0; i < nchannels; i++)
        outlet_new(&x->x_obj, gensym("signal"));
    x->x_noutlets = nchannels;
    x->x_bangout = outlet_new(&x->x_obj, &s_bang);
    pthread_mutex_init(&x->x_mutex, 0);
    pthread_cond_init(&x->x_requestcondition, 0);
    pthread_cond_init(&x->x_answercondition, 0);
    x->x_vecsize = MAXVECSIZE;
    x->x_state = STATE_IDLE;
    x->x_clock = clock_new(x, (t_method)readsf_tick);
    x->x_canvas = canvas_getcurrent();
    x->x_bytespersample = 2;
    x->x_sfchannels = 1;
    x->x_fd = -1;
    x->x_buf = buf;
    x->x_bufsize = bufsize;
    x->x_fifosize = x->x_fifohead = x->x_fifotail = x->x_requestcode = 0;
    pthread_create(&x->x_childthread, 0, readsf_child_main, x);
    return (x);
}

static void readsf_tick(t_readsf *x)
{
    outlet_bang(x->x_bangout);
}

static t_int *readsf_perform(t_int *w)
{
    t_readsf *x = (t_readsf *)(w[1]);
    int vecsize = x->x_vecsize, noutlets = x->x_noutlets, i, j,
        bytespersample = x->x_bytespersample,
        bigendian = x->x_bigendian,wantsamples;
    float *fp,tmp,speed=x->x_speed;
    if (x->x_state == STATE_STREAM)
    {
        int wantbytes, nchannels, sfchannels = x->x_sfchannels;
        pthread_mutex_lock(&x->x_mutex);
        tmp=vecsize *speed+x->x_frac;
        wantsamples=(int)tmp;
        x->x_frac=tmp-wantsamples;

        if(!speed) goto idle;

        wantbytes = sfchannels * wantsamples * bytespersample;

        while (
            !x->x_eof && x->x_fifohead >= x->x_fifotail &&
            x->x_fifohead < x->x_fifotail + wantbytes-1)
        {
            pute("wait...\n");
            sfread_cond_signal(&x->x_requestcondition);
            sfread_cond_wait(&x->x_answercondition, &x->x_mutex);
            pute("done\n");
        }
        if (x->x_eof && x->x_fifohead >= x->x_fifotail &&
                x->x_fifohead < x->x_fifotail + wantbytes-1)
        {
            if (x->x_fileerror)
            {
                pd_error(x, "dsp: %s: %s", x->x_filename,
                         (x->x_fileerror == EIO ?
                          "unknown or bad header format" :
                          strerror(x->x_fileerror)));
            }
            clock_delay(x->x_clock, 0);
            x->x_state = STATE_IDLE;
            sfread_cond_signal(&x->x_requestcondition);
            pthread_mutex_unlock(&x->x_mutex);
            goto idle;
        }

        if(speed==1)
            soundfile_xferin_sample(sfchannels, noutlets, x->x_outvec, 0,
                             (unsigned char *)(x->x_buf + x->x_fifotail), vecsize,
                             bytespersample, bigendian);
        else
        {
            soundfile_xferin_sample(sfchannels, noutlets, tmpvec, 0,
                             (unsigned char *)(x->x_buf + x->x_fifotail), wantsamples,
                             bytespersample, bigendian);
            interpolate(noutlets,tmpvec,wantsamples,x->x_outvec,vecsize);
        }

        x->x_fifotail += wantbytes;
        if (x->x_fifotail >= x->x_fifosize)
            x->x_fifotail = 0;
        if ((--x->x_sigcountdown) <= 0)
        {
            sfread_cond_signal(&x->x_requestcondition);
            x->x_sigcountdown = x->x_sigperiod;
        }
        pthread_mutex_unlock(&x->x_mutex);
    }
    else
    {
idle:
        for (i = 0; i < noutlets; i++)
            for (j = vecsize, fp = x->x_outvec[i]; j--; )
                *fp++ = 0;
    }
    return (w+2);
}

static void readsf_start(t_readsf *x)
{
    /* start making output.  If we're in the "startup" state change
    to the "running" state. */
    if (x->x_state == STATE_STARTUP)
        x->x_state = STATE_STREAM;
    else pd_error(x, "readsf: start requested with no prior 'open'");
}

static void readsf_stop(t_readsf *x)
{
    /* LATER rethink whether you need the mutex just to set a variable? */
    pthread_mutex_lock(&x->x_mutex);
    x->x_state = STATE_IDLE;
    x->x_requestcode = REQUEST_CLOSE;
    sfread_cond_signal(&x->x_requestcondition);
    pthread_mutex_unlock(&x->x_mutex);
}

static void readsf_float(t_readsf *x, t_floatarg f)
{
    if (f != 0)
        readsf_start(x);
    else readsf_stop(x);
}

static void readsf_speed(t_readsf *x, t_floatarg f)
{
    if((f>=0)&&(f<=8))
        x->x_speed=f;
}

/* open method.  Called as:
open filename [skipframes headersize channels bytespersamp endianness]
	(if headersize is zero, header is taken to be automatically
detected; thus, use the special "-1" to mean a truly headerless file.)
*/

static void readsf_open(t_readsf *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *filesym = atom_getsymbolarg(0, argc, argv);
    t_float onsetframes = atom_getfloatarg(1, argc, argv);
    t_float headerbytes = atom_getfloatarg(2, argc, argv);
    t_float channels = atom_getfloatarg(3, argc, argv);
    t_float bytespersamp = atom_getfloatarg(4, argc, argv);
    t_symbol *endian = atom_getsymbolarg(5, argc, argv);
    if (!*filesym->s_name)
        return;
    pthread_mutex_lock(&x->x_mutex);
    x->x_requestcode = REQUEST_OPEN;
    x->x_filename = filesym->s_name;
    x->x_fifotail = 0;
    x->x_fifohead = 0;
    if (*endian->s_name == 'b')
        x->x_bigendian = 1;
    else if (*endian->s_name == 'l')
        x->x_bigendian = 0;
    else if (*endian->s_name)
        pd_error(x, "endianness neither 'b' nor 'l'");
    else x->x_bigendian = garray_ambigendian();
    x->x_onsetframes = (onsetframes > 0 ? onsetframes : 0);
    x->x_skipheaderbytes = (headerbytes > 0 ? headerbytes :
                            (headerbytes == 0 ? -1 : 0));
    x->x_sfchannels = (channels >= 1 ? channels : 1);
    x->x_bytespersample = (bytespersamp > 2 ? bytespersamp : 2);
    x->x_eof = 0;
    x->x_fileerror = 0;
    x->x_state = STATE_STARTUP;
    x->x_speed=1;
    x->x_frac=0;
    sfread_cond_signal(&x->x_requestcondition);
    pthread_mutex_unlock(&x->x_mutex);
}

static void readsf_dsp(t_readsf *x, t_signal **sp)
{
    int i, noutlets = x->x_noutlets;
    pthread_mutex_lock(&x->x_mutex);
    x->x_vecsize = sp[0]->s_n;

    x->x_sigperiod = (x->x_fifosize /
                      (x->x_bytespersample * x->x_sfchannels * x->x_vecsize));
    for (i = 0; i < noutlets; i++)
        x->x_outvec[i] = sp[i]->s_vec;
    pthread_mutex_unlock(&x->x_mutex);
    dsp_add(readsf_perform, 1, x);
}

static void readsf_print(t_readsf *x)
{
    post("state %d", x->x_state);
    post("fifo head %d", x->x_fifohead);
    post("fifo tail %d", x->x_fifotail);
    post("fifo size %d", x->x_fifosize);
    post("fd %d", x->x_fd);
    post("eof %d", x->x_eof);
}

static void readsf_free(t_readsf *x)
{
    /* request QUIT and wait for acknowledge */
    void *threadrtn;
    pthread_mutex_lock(&x->x_mutex);
    x->x_requestcode = REQUEST_QUIT;
    post("stopping readsf thread...");
    sfread_cond_signal(&x->x_requestcondition);
    while (x->x_requestcode != REQUEST_NOTHING)
    {
        post("signalling...");
        sfread_cond_signal(&x->x_requestcondition);
        sfread_cond_wait(&x->x_answercondition, &x->x_mutex);
    }
    pthread_mutex_unlock(&x->x_mutex);
    if (pthread_join(x->x_childthread, &threadrtn))
        error("readsf_free: join failed");
    post("... done.");

    pthread_cond_destroy(&x->x_requestcondition);
    pthread_cond_destroy(&x->x_answercondition);
    pthread_mutex_destroy(&x->x_mutex);
    freebytes(x->x_buf, x->x_bufsize);
    clock_free(x->x_clock);
}

//#endif /* __linux__ */

void readsfv_tilde_setup(void)
{
//#ifdef __linux__
    int i;

    readsfv_class = class_new(gensym("readsfv~"), (t_newmethod)readsf_new,
                              (t_method)readsf_free, sizeof(t_readsf), 0, A_DEFFLOAT, A_DEFFLOAT, 0);

    class_addfloat(readsfv_class, (t_method)readsf_float);
    class_addmethod(readsfv_class, (t_method)readsf_speed, gensym("speed"), A_FLOAT,0);
    class_addmethod(readsfv_class, (t_method)readsf_start, gensym("start"), 0);
    class_addmethod(readsfv_class, (t_method)readsf_stop, gensym("stop"), 0);
    class_addmethod(readsfv_class, (t_method)readsf_dsp, gensym("dsp"), 0);
    class_addmethod(readsfv_class, (t_method)readsf_open, gensym("open"),
                    A_GIMME, 0);
    class_addmethod(readsfv_class, (t_method)readsf_print, gensym("print"), 0);

    for(i=0; i<MAXSFCHANS; i++)
        tmpvec[i]=getbytes(sizeof(t_sample)*8*1024);
//#endif /* __linux__ */
}



#endif /* NOT _WIN32 */
