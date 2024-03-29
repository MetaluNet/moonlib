#include <m_pd.h>
#include "g_canvas.h"

#ifdef _MSC_VER
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

#include <stdio.h>
#include <string.h>
#ifndef _WIN32
#include <unistd.h>
#include <sys/mman.h>
#else
#include <io.h>
#endif


#include <fcntl.h>
#include <sys/stat.h>

#include "_soundfile.c"

/* ------------------------ sfread~ ----------------------------- */

#ifdef _WIN32
#define BINREADMODE "rb"
#else
#define BINREADMODE "r"
#endif

static t_class *sfread_class;


typedef struct _sfread
{
    t_object x_obj;
    void     *x_mapaddr;
    int       x_fd;

    t_int   x_play;
    t_int   x_outchannels;
    t_int   x_loop;
    double x_index;	//samples
    t_float x_speed;	//1=sample rate
    t_int   x_interp;

    t_int   x_size;	// bytes
    t_int   x_fchannels; // file channels
    t_int   x_len;	// samples
    t_int x_skip;	//bytes
    t_clock *x_clock;


    t_glist *x_glist;
    t_outlet *x_stateout;
    t_outlet *x_sizeout;
} t_sfread;

#define MAX_CHANS 4

static t_int *sfread_perform(t_int *w)
{
    t_sfread *x = (t_sfread *)(w[1]);
    short *fp, *buf = NULL /*= x->x_mapaddr+x->x_skip*/;

    int c = x->x_outchannels;
    int fc = x->x_fchannels,fc2=2*fc;
    double findex = x->x_index;
    t_float speed = x->x_speed;
    float frac,  a,  b,  cc,  d, cminusb;
    int i,n,index,rindex;
    int end =  x->x_len -3;/* -3 is for safe interpolation*/
    t_float *out[MAX_CHANS];
    t_int in_off[MAX_CHANS];
    t_int loground=(fc==1?0:fc==2?1:2);

    //if(!x->x_mapaddr) return(w+c+4);

	for (i=0; i<c; i++)
	{
	    out[i] = (t_float *)(w[3+i]);
	    if(fc) in_off[i] = i%fc;
	    else in_off[i] = 0;
	}
	n = (int)(w[3+c]);

	if(x->x_mapaddr) {
		buf = x->x_mapaddr+x->x_skip;
		/* loop */

		if (findex >  end)
		    findex = end;

		if (findex + n*speed > end)   // playing forward end
		{
		    if (!x->x_loop)
		    {
		        x->x_play=0;
		        findex = 0;
		        clock_delay(x->x_clock, 0);
		    }
		}

		if (findex + n*speed < 1)    // playing backwards end
		{
		    if (!x->x_loop)
		    {
		        x->x_play=0;
		        findex = end;
		        clock_delay(x->x_clock, 0);
		    }

		}
	}

    if (x->x_play && x->x_mapaddr)
    {
        if (speed != 1)   /* different speed */
        {
            if (x->x_interp) while (n--)
                {
                    index=findex;
                    rindex=index<<loground;
                    frac = findex - index;
                    for (i=0; i<c; i++)
                    {
                        if(i > fc) *out[i]++ = 0;
                        else {
		                    fp=buf + rindex +in_off[i];
		                    a = fp[-fc];
		                    b = fp[0];
		                    cc = fp[fc];
		                    d = fp[fc2];
		                    cminusb = cc-b;
		                    *out[i]++ = 3.052689e-05*
		                                (b + frac * (cminusb - 0.5f * (frac-1.) *
		                                             ((a - d + 3.0f * cminusb) * frac + (b - a - cminusb))));
		                    //*out[i]++ = *(buf+rindex+in_off[i])*3.052689e-05;
	                    }
                    }
                    findex=findex+speed;
                    if (findex > end)
                    {
                        if (x->x_loop) findex = 1;
                        else break;
                    }
                    if (findex < 1)
                    {
                        if (x->x_loop) findex = end;
                        else break;
                    }
                }
            else while (n--)
                {
                    rindex=((int)findex)<<loground;
                    for (i=0; i<c; i++)
                    {
                        if(i > fc) *out[i]++ = 0;
                        else *out[i]++ = *(buf+rindex+in_off[i])*3.052689e-05;
                    }
                    findex+=speed;
                    if (findex > end)
                    {
                        if (x->x_loop) findex = 1;
                        else break;
                    }
                    if (findex < 1)
                    {
                        if (x->x_loop) findex = end;
                        else break;
                    }
                }
            /* Fill with zero in case of end */
            n++;
            while (n--)
                for (i=0; i<c; i++)
                    *out[i]++ = 0;
            //offset = aoff;
        }
        else   /* speed == 1 */
        {
            int end2=end*fc;
            rindex=((int)findex)<<loground;
            while (n--)
            {
                for (i=0; i<c; i++)
                {
                    if(i > fc) *out[i]++ = 0;
                    else *out[i]++ = *(buf+rindex+in_off[i])*3.052689e-05;
                }
                rindex+=fc;
                if (rindex > end2)
                {
                    if (x->x_loop) rindex = 1;
                    else break;
                }
            }

            /* Fill with zero in case of end */
            n++;
            while (n--)
                for (i=0; i<c; i++)
                    *out[i]++ = 0.;
            findex=rindex>>loground;
        }

    }
    else
    {
        while (n--)
        {
            for (i=0; i<c; i++)
                *out[i]++ = 0.;
        }
    }
    x->x_index = findex;
    return (w+c+4);
}


static void sfread_loop(t_sfread *x, t_floatarg f)
{
    x->x_loop = f;
}

static void sfread_interp(t_sfread *x, t_floatarg f)
{
    x->x_interp = (f!=0);
}

static void sfread_index(t_sfread *x, t_symbol* s, int argc, t_atom* argv)
{
  if (argc > 0 && argv[0].a_type == A_FLOAT){
    x->x_index = argv[0].a_w.w_float;
  }
  t_atom a;
  SETFLOAT(&a, x->x_index);
  outlet_anything(x->x_stateout, s, 1, &a);
}

static void sfread_size(t_sfread *x)
{
    outlet_float(x->x_sizeout,x->x_len);
}

static void sfread_state(t_sfread *x)
{
    outlet_float(x->x_stateout, x->x_play);
}

static void sfread_float(t_sfread *x, t_floatarg f)
{
    int t = f;
    if (t && x->x_mapaddr)
    {
        x->x_play=1;
    }
    else
    {
        x->x_play=0;
    }
    sfread_state(x);
}


static void sfread_bang(t_sfread *x)
{
    x->x_index = x->x_speed>0?1:x->x_len-3;
    sfread_float(x,1.0);
}


static void sfread_dsp(t_sfread *x, t_signal **sp)
{
    switch (x->x_outchannels)
    {
    case 1:
        dsp_add(sfread_perform, 4, x, sp[0]->s_vec,
                sp[1]->s_vec, sp[0]->s_n);
        break;
    case 2:
        dsp_add(sfread_perform, 5, x, sp[0]->s_vec,
                sp[1]->s_vec,sp[2]->s_vec, sp[0]->s_n);
        break;
    case 4:
        dsp_add(sfread_perform, 7, x, sp[0]->s_vec,
                sp[1]->s_vec,sp[2]->s_vec,
                sp[3]->s_vec,sp[4]->s_vec,
                sp[0]->s_n);
        break;
    }
}


static void sfread_open(t_sfread *x, t_symbol *filename)
{
    struct stat fstate;
    char fname[MAXPDSTRING];

    t_soundfile_info info;

	info.samplerate = 0;
    info.channels = 0;
    info.bytespersample = 0;
    info.headersize = -1;
    info.bigendian = 0;
    info.bytelimit = 0x7fffffff;

    if (filename == &s_)
    {
        post("sfread: open without filename");
        return;
    }

    canvas_makefilename(glist_getcanvas(x->x_glist), filename->s_name,
                        fname, MAXPDSTRING);

    /* close the old file */

    if (x->x_mapaddr) munmap(x->x_mapaddr, x->x_size);
    if (x->x_fd >= 0) close(x->x_fd);

    if ((x->x_fd = open_soundfile("", fname, &info, 0)) < 0)
    {
        pd_error(x, "can't open %s", fname);
        x->x_play = 0;
        x->x_mapaddr = NULL;
        return;
    }

    if( ((info.channels != 1) && (info.channels != 2) && (info.channels != 4)) ||
            (info.bytespersample != 2) )
    {
        pd_error(x, "file %s error: not a 1 or 2 or 4 channels soundfile, or not a 16 bits soundfile",fname);
        post("channels:%d bytes:%d ", info.channels, info.bytespersample);
        x->x_play = 0;
        x->x_mapaddr = NULL;
        return;
    }

    x->x_fchannels = info.channels;

    /* get the size */

    fstat(x->x_fd, &fstate);
    x->x_size = (int)fstate.st_size;
    x->x_skip = x->x_size - info.bytelimit;
    x->x_len = (info.bytelimit) / (info.bytespersample*x->x_fchannels);

    /* map the file into memory */

    if (!(x->x_mapaddr = mmap(NULL, x->x_size, PROT_READ, MAP_PRIVATE, x->x_fd, 0)))
    {
        pd_error(x, "can't mmap %s", fname);
        return;
    }
    sfread_size(x);
}

static void *sfread_new(t_floatarg chan,t_floatarg interp)
{
    t_sfread *x = (t_sfread *)pd_new(sfread_class);
    t_int c = chan;

    x->x_glist = (t_glist *) canvas_getcurrent();

    if (c<1 || c > MAX_CHANS) c = 1;
    floatinlet_new(&x->x_obj, &x->x_speed);


    x->x_fd = -1;
    x->x_mapaddr = NULL;

    x->x_size = 0;
    x->x_len = 0;
    x->x_loop = 0;
    x->x_outchannels = c;
    x->x_fchannels = 0;
    x->x_mapaddr = NULL;
    x->x_index = 1;
    x->x_skip = 0;
    x->x_speed = 1.0;
    x->x_play = 0;
    x->x_interp = (interp!=0);
    x->x_clock = clock_new(x, (t_method)sfread_state);

    while (c--)
    {
        outlet_new(&x->x_obj, gensym("signal"));
    }

    x->x_stateout = outlet_new(&x->x_obj, &s_float);
    x->x_sizeout = outlet_new(&x->x_obj, &s_float);

    /*  post("sfread: x_channels = %d, x_speed = %f",x->x_channels,x->x_speed);*/

    return (x);
}

static void sfread_free(t_sfread *x)
{
    clock_free(x->x_clock);
}

void sfread2_tilde_setup(void)
{
    /* sfread */

    sfread_class = class_new(gensym("sfread2~"), (t_newmethod)sfread_new,
                             (t_method)sfread_free,sizeof(t_sfread), 0,A_DEFFLOAT,A_DEFFLOAT,0);

    class_addmethod(sfread_class, nullfn, gensym("signal"), 0);
    class_addmethod(sfread_class, (t_method) sfread_dsp, gensym("dsp"), 0);
    class_addmethod(sfread_class, (t_method) sfread_open, gensym("open"), A_SYMBOL,A_NULL);
    class_addmethod(sfread_class, (t_method) sfread_size, gensym("size"), 0);
    class_addmethod(sfread_class, (t_method) sfread_state, gensym("state"), 0);
    class_addfloat(sfread_class, sfread_float);
    class_addbang(sfread_class,sfread_bang);
    class_addmethod(sfread_class,(t_method)sfread_loop,gensym("loop"),A_FLOAT,A_NULL);
    class_addmethod(sfread_class,(t_method)sfread_interp,gensym("interp"),A_FLOAT,A_NULL);
    class_addmethod(sfread_class,(t_method)sfread_index,gensym("index"),A_GIMME,A_NULL);


    // Impossible with pd-0.35 because it leaves super-user mode.
    //if(munlockall()) perror("munlockall()");
    //if(mlockall(MCL_CURRENT)) perror("mlockall(MCL_CURRENT)");
}


