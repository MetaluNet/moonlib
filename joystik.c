#ifdef __gnu_linux__
/*
'pd_joystik' (An external library for Miller Puckette's 'PD' software
adding PC and/or USB joystik control capabilities)

Copyright (C) 2001 Antoine Rousseau (from the initial file of Joseph A. Sarlo)

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
#include <s_stuff.h>

#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <errno.h>
#include <linux/joystick.h>
#include <glib.h>

/*0=normal joystick: /dev/jsXX		1=usb joystick: /dev/input/jsXX */

static GList *(joy_lists)[2][16]={{0},{0}};
static int fds[2][16];

typedef struct _joystik
{
    t_object t_ob;
    t_outlet *x_axis_out;
    t_outlet *x_button_out;
	 guchar x_connected;
    guchar x_usb; /*0:normal 1:usb*/
    gushort x_number; /*0 to 15*/
}t_joystik;

t_class *joystik_class;

void joystik_setup(void);
static void joystik_read(GList *l,int fd);

static void fd_open(int usb,int n)
{
	char filename[128];
	int *fd=&fds[(int)usb][(int)n];
	GList *l=joy_lists[(int)usb][(int)n];

	if(usb) sprintf(filename,"/dev/input/js%d",n);
	else sprintf(filename,"/dev/js%d",n);

	if(*fd<=0){
		*fd = open (filename, O_RDONLY | O_NONBLOCK);
		if(*fd<0){
			post("open (%s, O_RDONLY | O_NONBLOCK)",filename);
			perror("open");
			return;
		}
	}
	sys_addpollfn(*fd,(t_fdpollfn)joystik_read,(void*)l);
	/*post("joystick: adding poll");*/
}

static void fd_close(int usb,int n)
{
	int *fd=&fds[(int)usb][(int)n];

	sys_rmpollfn(*fd);
	close(*fd);
	*fd=-1;
	/*post("joystick: removing poll");*/
}

static void joystik_connect(t_joystik *x)
{
	GList *l=joy_lists[x->x_usb][x->x_number];

	if(g_list_find(l,(void*)x)) return;

	g_list_append(l,(void*)x);

	if(g_list_length(l)==2){
		fd_open(x->x_usb,x->x_number);
	}
}
static void joystik_deconnect(t_joystik *x)
{
	GList *l=joy_lists[x->x_usb][x->x_number];

	if(!g_list_find(l,(void*)x)) return;

	g_list_remove(l,(void*)x);

	if(g_list_length(l)==1){
		fd_close(x->x_usb,x->x_number);
	}
}

static void joystik_float(t_joystik *x,t_floatarg connect)
{
	connect=(connect!=0);

	if(x->x_connected==connect) return;

	if(connect) joystik_connect(x);
	else joystik_deconnect(x);

	x->x_connected=connect;
}

static void *joystik_new(t_float num, t_float usb)
{  
	t_joystik *x = (t_joystik *)pd_new(joystik_class);
	int n=num;
	GList **l=0;

	if((n<0)||(n>16)) {
		post("ERROR in joystik_new : device number out of range");
		return (void*)x;
	}
	
	usb=(usb!=0);
	x->x_axis_out = outlet_new(&x->t_ob, &s_list);
	x->x_button_out = outlet_new(&x->t_ob, &s_list);
	
	x->x_connected=0;
	x->x_usb=usb;
	x->x_number=n;

	l=&joy_lists[(int)usb][(int)n];

	if(!*l) *l=g_list_alloc();
	
	return (void *)x;
}

void joystik_setup(void)
{
	joystik_class = class_new(gensym("joystik"),(t_newmethod)joystik_new, 
		(t_method)joystik_deconnect, sizeof(t_joystik), 0, A_DEFFLOAT, A_DEFFLOAT, 0);
	class_addfloat(joystik_class, joystik_float);
	

}

static void joystik_read(GList *l,int fd)
{
	#define AMP 32767.0
	int rt;
	struct js_event e;
	t_atom ats[2];
	GList *ltmp;
	
	while (read (fd, &e, sizeof(struct js_event)) > -1)
	{
		SETFLOAT(&ats[0],e.number);
		if (e.type == JS_EVENT_AXIS)
		{
			SETFLOAT(&ats[1],e.value/AMP);
			ltmp=l;
			while(ltmp->next){
				ltmp=ltmp->next;
				outlet_list(((t_joystik*)ltmp->data)->x_axis_out,0,2,ats);
			}
		}
		else if (e.type == JS_EVENT_BUTTON)
		{
			SETFLOAT(&ats[1],e.value);
			ltmp=l;
			while(ltmp->next){
				ltmp=ltmp->next;
				outlet_list(((t_joystik*)ltmp->data)->x_button_out,0,2,ats);
			}
		}
	}
}

#endif /* __gnu_linux__ */
