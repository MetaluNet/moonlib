/* ------------------------- ndndmetro   -----------------------------------------*/
/*                                                                              */
/*  Antoine Rousseau 2005-2016                                                       */
/*                                                                              */
/*
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
/* ---------------------------------------------------------------------------- */

#include "m_pd.h"
#include <stdio.h>
#include <string.h>
#ifdef UNIX
#include <stdlib.h>
#endif
#include <math.h>

static t_class *ndmetro_class;


typedef struct _ndmetro
{
    t_object x_obj;
    t_clock *x_clock;
    double x_tempo;
    double x_newtempo;
    double x_num;
    double x_denum;
    int x_tick;
    int x_hit;
    int x_on;
    double x_lasttime;
    double x_lastbeat;
    t_outlet *x_floatoutlet;
} t_ndmetro;

static void ndmetro_tick(t_ndmetro *x)
{
    double dtime,nextime,beat,tick=x->x_tick,tickk,fracpart=0;
    double intpart;

    x->x_hit = 0;

    if(tick>=0){
	outlet_float(x->x_obj.ob_outlet,fmod(tick,x->x_num));
	if (!x->x_hit) {
		clock_delay(x->x_clock, x->x_tempo/x->x_denum);
    		x->x_tick=tick+1;
	}
    }
    else{
    	dtime=clock_gettimesince(x->x_lasttime);
    	beat=x->x_lastbeat+dtime/x->x_tempo;
    	tick=beat*x->x_denum;
    	if(fabs((tickk=rint(tick))-tick)<1e-6) tick=tickk;

    	fracpart=modf(tick,&intpart);

   //fprintf(stderr,"metro_tick; beat=%.15g tick=%.15g fracpart=%.15g\n",beat,tick,fracpart);
    	if(!fracpart) outlet_float(x->x_obj.ob_outlet,fmod(tick,x->x_num));
	else  outlet_float(x->x_floatoutlet,fmod(tick,x->x_num));

    	if (!x->x_hit) {
    		x->x_tempo=x->x_newtempo;
    		x->x_lastbeat=beat;
    		x->x_lasttime=clock_getsystime();
    		nextime=x->x_tempo*(1.0-fracpart)/x->x_denum;
    		if(nextime<0.01) nextime=0.01;
		clock_delay(x->x_clock, nextime);
    		x->x_tick=floor(tick)+1;
	}
    }
}

static void ndmetro_bang(t_ndmetro *x)
{
    //post("metro_bang");
    x->x_tick=-1;
    if(x->x_on) ndmetro_tick(x);
    x->x_hit = 1;
}

static void ndmetro_list(t_ndmetro *x, t_symbol *s,  int ac, t_atom *av)
{
    double beat,tempo;
    t_atom *ap=av;

    //post("metro_list");

    if(ac<1) {
	ndmetro_bang(x);
	return;
    }

    if(ap->a_type!=A_FLOAT) {
	ndmetro_bang(x);
	return;
    }

    beat=atom_getfloat(ap);ap++;


    //fprintf(stderr,"beat=%g\n",beat);
    if(beat==-1) {
    	clock_unset(x->x_clock);
	x->x_on=0;
	x->x_hit = 1;
	//x->x_tick=0;
	return;
    }

    if((ac>1)&&(ap->a_type==A_FLOAT)) {
    	tempo=atom_getfloat(ap);
    	if(tempo<1) tempo=1;
    	x->x_newtempo=tempo;
    //fprintf(stderr,"tempo=%f\n",tempo);
    }

    x->x_lastbeat=beat;
    x->x_lasttime=clock_getsystime();
    x->x_on=1;

    ndmetro_bang(x);
}


static void ndmetro_nd(t_ndmetro *x, t_floatarg n,t_floatarg d)
{
    if(d<0.01) d=0.01;
    if(n<0.01) n=0.01;
    x->x_num=n;
    x->x_denum=d;
    x->x_tick=-1;
    //if(x->x_on) ndmetro_tick(x);
    ndmetro_bang(x);

}

static void ndmetro_tempo(t_ndmetro *x, t_floatarg t)
{
    if(t<1) t=1;
    x->x_newtempo=t;
    x->x_tick=-1;
    //if(x->x_on) ndmetro_tick(x);
    ndmetro_bang(x);
}

static void ndmetro_free(t_ndmetro *x)
{
    clock_free(x->x_clock);
}

static void *ndmetro_new(t_floatarg n, t_floatarg d)
{
    t_ndmetro *x = (t_ndmetro *)pd_new(ndmetro_class);

    if(d==0) d=1;
    if(n==0) n=1;
    if(d<0.01) d=0.01;
    if(n<0.01) n=0.01;
    x->x_num=n;
    x->x_denum=d;
    x->x_hit = 0;
    x->x_on = 0;
    x->x_clock = clock_new(x, (t_method)ndmetro_tick);
    x->x_tempo=x->x_newtempo=1000;
    x->x_lasttime=clock_getsystime();
    x->x_lastbeat=0;
    outlet_new(&x->x_obj, gensym("float"));
    x->x_floatoutlet=outlet_new(&x->x_obj, gensym("float"));
    //inlet_new(&x->x_obj, &x->x_obj.ob_pd, gensym("float"), gensym("ft1"));
    return (x);
}

void ndmetro_setup(void)
{
    ndmetro_class = class_new(gensym("ndmetro"), (t_newmethod)ndmetro_new,
        (t_method)ndmetro_free, sizeof(t_ndmetro), 0, A_DEFFLOAT,  A_DEFFLOAT, 0);
    class_addbang(ndmetro_class, (t_method)ndmetro_bang);
    class_addlist(ndmetro_class, (t_method)ndmetro_list);
    class_addmethod(ndmetro_class, (t_method)ndmetro_nd, gensym("nd"),
        A_FLOAT,A_FLOAT, 0);
    class_addmethod(ndmetro_class, (t_method)ndmetro_tempo, gensym("tempo"),
        A_FLOAT, 0);
}
