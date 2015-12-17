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

#include <m_pd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h> 

#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
#define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)

static t_class *f2s_class;

typedef struct _f2s
{
   t_object x_ob;
	t_float x_f;
	int x_type; /*0: p/n/µ/m/./k/M/G with n digits */
	int x_n;
} t_f2s;

static void f2s_bang(t_f2s *x)
{
	char buf[256],delim;
	float f=x->x_f,fnorm;
	int ilogf,ilogf3,fh,fl=0,subdigits;
	
	if(!f) {
		sprintf(buf,"0");
		goto end;
	}
	ilogf=(int)floor(log10(fabs(f)));
	ilogf3=(int)floor(log10(fabs(f))/3);
	
	subdigits=2-ilogf+ilogf3*3;
	
	fnorm=f*pow(10,-ilogf3*3);
	fh=(int)(fnorm);
	fl=(int)(fabs((fnorm-fh)*pow(10,subdigits))+0.00001);

	switch(ilogf3){
		case -4: delim='p'; break;
		case -3: delim='n'; break;
		case -2: delim='u'; break;
		case -1: delim='m'; break;
		case 0: delim=subdigits?'.':0; break;
		case 1: delim='k'; break;
		case 2: delim='M'; break;
		case 3: delim='G'; break;
		case 4: delim='T'; break;
		default:delim='?';
	}
	
	if(subdigits)
		sprintf(buf,"%i%c%0*i",fh,delim,subdigits,fl);
	else {
		if(delim=='m')
			sprintf(buf,"%s%03i",fh<0?"-.":".",abs(fh));
		else sprintf(buf,"%i%c",fh,delim);
	}
	
	end:
	outlet_symbol(((t_object *)x)->ob_outlet,gensym(buf));
}

static void f2s_float(t_f2s *x,t_float val)
{
	x->x_f=val;
	f2s_bang(x);
}

static void *f2s_new( t_symbol *s, int argc, t_atom *argv)
{
	t_atom *at;

	t_f2s *x=(t_f2s*)pd_new(f2s_class);	

	x->x_f=0;
	x->x_type=0;
	x->x_n=3;
	
	outlet_new(&x->x_ob, &s_symbol);
	
	if(argc&&IS_A_FLOAT(argv,0)) {
		x->x_n=atom_getfloat(argv++);
		argc--;
	}
	
	return (void *)x;
}

void f2s_setup(void)
{
	f2s_class = class_new(gensym("f2s"),(t_newmethod)f2s_new, 
		0, sizeof(t_f2s),0,A_GIMME ,0);

	class_addfloat(f2s_class, (t_method)f2s_float);
	class_addbang(f2s_class, (t_method)f2s_bang);
	class_sethelpsymbol(f2s_class, gensym("moonlibs/f2s"));
}

