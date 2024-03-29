/*
Copyright (C) 2002 Antoine Rousseau

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Lesser General Public
License as published by the Free Software Foundation; either
version 2.1 of the License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Lesser General Public License for more details.

You should have received a copy of the GNU Lesser General Public
License along with this library; if not, write to the Free Software
Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

*/

#include "m_pd.h"
#include "math.h"


/* ---------------- tabenv - table envelope computer. ----------------- */
/*------- (in fact it's a mix between env~ and tabplay~)----------------*/

#define MAXOVERLAP 10
#define MAXVSTAKEN 64

typedef struct tabenv
{
    /*env part*/
    t_object x_obj; 	    	    /* header */
    t_outlet *x_outlet;		    /* a "float" outlet */
    float *x_buf;		    /* a Hanning window */
    int x_phase;		    /* number of points since last output */
    int x_period;		    /* requested period of output */
    int x_realperiod;		    /* period rounded up to vecsize multiple */
    int x_npoints;		    /* analysis window size in samples */
    float x_result;		    /* result to output */
    float x_sumbuf[MAXOVERLAP];	    /* summing buffer */

    /*tabplay part*/
    int x_tabphase;
    int x_nsampsintab;
    int x_limit;
    t_word *x_vec;
    t_symbol *x_arrayname;
} t_tabenv;

t_class *tabenv_class;

static void *tabenv_new(t_symbol *s,t_floatarg fnpoints, t_floatarg fperiod)
{
    int npoints = fnpoints;
    int period = fperiod;
    t_tabenv *x;
    float *buf;
    int i;

    if (npoints < 1) npoints = 1024;
    if (period < 1) period = npoints/2;
    if (period < npoints / MAXOVERLAP + 1)
        period = npoints / MAXOVERLAP + 1;
    if (!(buf = getbytes(sizeof(float) * (npoints + MAXVSTAKEN))))
    {
        pd_error(0,"env: couldn't allocate buffer");
        return (0);
    }
    x = (t_tabenv *)pd_new(tabenv_class);
    x->x_buf = buf;
    x->x_npoints = npoints;
    x->x_phase = 0;
    x->x_period = period;
    for (i = 0; i < MAXOVERLAP; i++) x->x_sumbuf[i] = 0;
    for (i = 0; i < npoints; i++)
        buf[i] = (1. - cos((2 * 3.14159 * i) / npoints))/npoints;
    for (; i < npoints+MAXVSTAKEN; i++) buf[i] = 0;
    x->x_outlet = outlet_new(&x->x_obj, gensym("float"));

    /* tabplay */
    x->x_tabphase = 0x7fffffff;
    x->x_limit = 0;
    x->x_arrayname = s;

    return (x);
}

static void tabenv_perform_64(t_tabenv *x,t_word *in)
{
    int n = 64;
    int count;
    float *sump;
    in += n;
    for (count = x->x_phase, sump = x->x_sumbuf;
            count < x->x_npoints; count += x->x_realperiod, sump++)
    {
        float *hp = x->x_buf + count;
        t_word *fp = in;
        float sum = *sump;
        int i;

        for (i = 0; i < n; i++)
        {
            fp--;
            sum += *hp++ * ((*fp).w_float * (*fp).w_float);
        }
        *sump = sum;
    }
    sump[0] = 0;
    x->x_phase -= n;
    if (x->x_phase < 0)
    {
        x->x_result = x->x_sumbuf[0];
        for (count = x->x_realperiod, sump = x->x_sumbuf;
                count < x->x_npoints; count += x->x_realperiod, sump++)
            sump[0] = sump[1];
        sump[0] = 0;
        x->x_phase = x->x_realperiod - n;
        outlet_float(x->x_outlet, powtodb(x->x_result));
    }
}


static void tabenv_set(t_tabenv *x, t_symbol *s)
{
    t_garray *a;

    x->x_arrayname = s;
    if (!(a = (t_garray *)pd_findbyclass(x->x_arrayname, garray_class)))
    {
        if (*s->s_name) pd_error(x, "tabenv: %s: no such array",
                                     x->x_arrayname->s_name);
        x->x_vec = 0;
    }
    else if (!garray_getfloatwords(a, &x->x_nsampsintab, &x->x_vec))
    {
        pd_error(x, "%s: bad template for tabenv", x->x_arrayname->s_name);
        x->x_vec = 0;
    }
}

static void tabenv_list(t_tabenv *x, t_symbol *s,
                        int argc, t_atom *argv)
{
    long start = atom_getfloatarg(0, argc, argv);
    long length = atom_getfloatarg(1, argc, argv);
    t_word *limitp, *p;

    tabenv_set(x, x->x_arrayname);

    if (start < 0) start = 0;
    if (length <= 0)
        x->x_limit = 0x7fffffff;
    else
        x->x_limit = start + length;
    x->x_tabphase = start;

    if(length <= 0) length = x->x_nsampsintab - 1;
    if(start >= x->x_nsampsintab) start = x->x_nsampsintab - 1;
    if((start + length) >= x->x_nsampsintab)
        length = x->x_nsampsintab - 1 - start;

    limitp = x->x_vec + start + length - 63;
    x->x_realperiod = x->x_period;

    for(p = x->x_vec + start; p < limitp ; p += 64)
        tabenv_perform_64( x , p );
}

static void tabenv_reset(t_tabenv *x)
{
    int i;
    x->x_phase = 0;
    for (i = 0; i < MAXOVERLAP; i++) x->x_sumbuf[i] = 0;
}

static void tabenv_ff(t_tabenv *x)		/* cleanup on free */
{
    freebytes(x->x_buf, (x->x_npoints + MAXVSTAKEN) * sizeof(float));
}


void tabenv_setup(void )
{
    tabenv_class = class_new(gensym("tabenv"), (t_newmethod)tabenv_new,
                             (t_method)tabenv_ff, sizeof(t_tabenv), 0, A_DEFSYM, A_DEFFLOAT, A_DEFFLOAT, 0);
    class_addmethod(tabenv_class, (t_method)tabenv_reset,
                    gensym("reset"), 0);
    class_addmethod(tabenv_class, (t_method)tabenv_set,
                    gensym("set"), A_DEFSYM, 0);
    class_addlist(tabenv_class, tabenv_list);

}
