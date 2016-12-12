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
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <m_pd.h>
#include "g_canvas.h"

#ifdef NT
#pragma warning( disable : 4244 )
#pragma warning( disable : 4305 )
#endif

/* ------------------------ lcdbitmap ----------------------------- */
#define BACKGROUND "-fill grey"
#define BACKGROUNDCOLOR "grey"


#define IS_A_POINTER(atom,index) ((atom+index)->a_type == A_POINTER)
#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
#define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)


//#define DEFAULTSIZE 3
//#define DEFAULTWIDTH 366
//#define DEFAULTHEIGHT 96
//#define DEFAULTWIDTH 244
//#define DEFAULTHEIGHT 64
#define DEFAULTWIDTH 122
#define DEFAULTHEIGHT 32

#define DEFAULTCOLOR "black"
#define BLACKCOLOR "black"
#define WHITECOLOR "white"
#define SELBLACKCOLOR "gold"
#define SELWHITECOLOR "yellow"


static t_class *lcdbitmap_class;

#define te_xpos te_xpix
#define te_ypos te_ypix

typedef struct _lcdbitmap
{
	t_object x_obj;
	t_glist * x_glist;
	int x_width;
	int x_height;
	int x_x;
	int x_page;
	int x_side;
	char x_bitmap[122][32];
	char *x_bitmapstr;
	char x_bitstr[65536];	     
	t_int  x_localimage; //localimage "img%x" done 
} t_lcdbitmap;

/* widget helper functions */

static char *Hurlo="\
#define BM_width 122\\n\
#define BM_height 32\\n\
static unsigned char BM_bits[] = {\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xc0, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0xc0, 0x01, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0xc0, 0x03, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x80, 0x03, 0x0e, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x80, 0x07, 0x0e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x80, 0x07, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0f, 0x0f, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x1c, 0x86, 0x0f, 0x00, 0x00, 0x03, 0x00, 0x00,\n\
   0x00, 0x8f, 0x0f, 0x00, 0x00, 0x00, 0x00, 0x00, 0x3c, 0x86, 0x0f, 0x00,\n\
   0x80, 0x07, 0x00, 0x00, 0x00, 0x9f, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0xfc, 0xc7, 0x0f, 0x00, 0xc0, 0x07, 0x00, 0x00, 0x00, 0xff, 0x0d, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0xcc, 0xc7, 0x0f, 0x00, 0xc0, 0x0f, 0x00, 0x00,\n\
   0x00, 0xfe, 0x0d, 0x00, 0x00, 0x00, 0x00, 0x00, 0x0c, 0xc6, 0x0e, 0x00,\n\
   0xc0, 0x1e, 0x00, 0x03, 0x00, 0xfe, 0x0c, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x0c, 0xc6, 0x0e, 0x00, 0xc0, 0x1e, 0x80, 0x07, 0x00, 0x6e, 0x0c, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0xc7, 0x0e, 0x00, 0xc0, 0x1e, 0xc0, 0x8f,\n\
   0x01, 0x6e, 0x0e, 0x00, 0x0c, 0x00, 0x00, 0x00, 0x00, 0xc7, 0x06, 0x00,\n\
   0xc0, 0x1c, 0xe0, 0xcf, 0x00, 0x0e, 0x0e, 0x00, 0x1c, 0x20, 0x00, 0x00,\n\
   0x00, 0xc7, 0x07, 0x00, 0xc0, 0x1c, 0xf0, 0xff, 0x00, 0x0c, 0x0e, 0x00,\n\
   0x1c, 0x70, 0x00, 0x00, 0x00, 0xc7, 0x07, 0xc0, 0xc0, 0x1c, 0xf8, 0x7d,\n\
   0x00, 0x0c, 0x06, 0x00, 0x1c, 0xf8, 0x00, 0x00, 0x00, 0xc7, 0x03, 0xc0,\n\
   0xc0, 0x1c, 0x78, 0x3c, 0x00, 0x0c, 0x06, 0xc0, 0x1c, 0x70, 0x00, 0x00,\n\
   0xe0, 0xff, 0x01, 0xcc, 0x9f, 0x1c, 0x3c, 0x70, 0x00, 0x1c, 0x07, 0xf8,\n\
   0xfc, 0x23, 0xe0, 0x01, 0x60, 0xc3, 0x70, 0xcc, 0x9f, 0x0d, 0x1c, 0x70,\n\
   0x80, 0x1c, 0x67, 0xfe, 0xfc, 0x03, 0xf0, 0x03, 0x00, 0xc3, 0x70, 0x8c,\n\
   0x91, 0x05, 0x1c, 0x60, 0xc0, 0x1d, 0xf7, 0xff, 0x0c, 0x30, 0x78, 0x03,\n\
   0x80, 0xc3, 0x70, 0x8c, 0x19, 0x07, 0x1c, 0x60, 0xe0, 0x1d, 0xf7, 0xe7,\n\
   0x0c, 0x30, 0x3c, 0x00, 0x80, 0xc3, 0x70, 0x8c, 0x19, 0x03, 0x0c, 0x70,\n\
   0x60, 0x1c, 0xf3, 0xc3, 0x0c, 0x30, 0x1c, 0x00, 0x9c, 0xc3, 0x70, 0xcc,\n\
   0x1c, 0xc7, 0x1d, 0x70, 0x67, 0x1c, 0xe3, 0xc1, 0x1c, 0x30, 0x0e, 0x00,\n\
   0x8e, 0xc3, 0x71, 0xcc, 0x9c, 0xc5, 0x39, 0x78, 0xe7, 0x1e, 0xf7, 0xc3,\n\
   0x1d, 0xb8, 0x1f, 0x00, 0x86, 0xc3, 0x73, 0xfe, 0xfc, 0x1d, 0x78, 0x3c,\n\
   0xc0, 0x1f, 0xbf, 0xef, 0xfd, 0xff, 0xf9, 0x01, 0xce, 0x81, 0xff, 0xff,\n\
   0xfc, 0x1c, 0xf0, 0x1f, 0xc0, 0x07, 0x1e, 0xff, 0xff, 0xef, 0xf0, 0x01,\n\
   0xfc, 0x01, 0xef, 0xcf, 0x78, 0x18, 0xf0, 0x1f, 0x80, 0x03, 0x0e, 0xfc,\n\
   0xff, 0xe3, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,\n\
   0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };";

static void draw_inlets(t_lcdbitmap *x, t_glist *glist, int firsttime, int nin, int nout)
{
     int n = nout;
     int nplus, i;
    int xpos=text_xpix(&x->x_obj, glist);
    int ypos=text_ypix(&x->x_obj, glist);

     nplus = (n == 1 ? 1 : n-1);
     for (i = 0; i < n; i++)
     {
	  int onset = xpos + (x->x_width - IOWIDTH) * i / nplus;
	  if (firsttime)
	       sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xo%d\n",
			glist_getcanvas(glist),
			onset, ypos + x->x_height - 1,
			onset + IOWIDTH, ypos + x->x_height,
			x, i);
	  else
	       sys_vgui(".x%x.c coords %xo%d %d %d %d %d\n",
			glist_getcanvas(glist), x, i,
			onset, ypos + x->x_height - 1,
			onset + IOWIDTH, ypos + x->x_height);
     }
     n = nin; 
     nplus = (n == 1 ? 1 : n-1);
     for (i = 0; i < n; i++)
     {
	  int onset = xpos + (x->x_width - IOWIDTH) * i / nplus;
	  if (firsttime)
	       sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xi%d\n",
			glist_getcanvas(glist),
			onset, ypos,
			onset + IOWIDTH, ypos + 1,
			x, i);
	  else
	       sys_vgui(".x%x.c coords %xi%d %d %d %d %d\n",
			glist_getcanvas(glist), x, i,
			onset, ypos,
			onset + IOWIDTH, ypos + 1);
	  
     }
}

void lcdbitmap_drawme(t_lcdbitmap *x, t_glist *glist, int firsttime)
{
     int i,j;
	  float x1,y1,x2,y2;
	  int xi1,yi1,xi2,yi2;
	  char *color;
    int xpos=text_xpix(&x->x_obj, glist);
    int ypos=text_ypix(&x->x_obj, glist);
	  
    if (firsttime) {
	sys_vgui(".x%x.c create rectangle %d %d %d %d -tags %xS "BACKGROUND"\n",
		glist_getcanvas(glist),
		xpos, ypos,
		xpos + x->x_width, ypos + x->x_height,
		x);
	  
     }     
     else {
	sys_vgui(".x%x.c coords %xS %d %d %d %d\n",
		glist_getcanvas(glist), x,
		xpos, ypos,
		xpos + x->x_width, ypos + x->x_height);
     }
	
     if (firsttime) {		
	if(!x->x_localimage) {
		sys_vgui("image create bitmap img%x\n",x);
		sys_vgui("image create bitmap imgnull\n",x);
			x->x_localimage=1;
	}
	sys_vgui("img%x configure -data \"%s\"\n",x,x->x_bitstr);
	//post("img%x configure -data \" %s \" \n",x,x->x_bitstr);
	/*char teststr[65536];
	sprintf(teststr,"img%x configure -data \"%s\" \n",x,x->x_bitstr);
	//fprintf(stderr,"%s\n",teststr);
	sys_vgui(teststr);
	//post("%s",Hurlo);*/

	sys_vgui(".x%x.c create image %d %d -image img%x -tags %xI\n", 
		glist_getcanvas(glist),xpos + x->x_width/2,ypos+ x->x_height/2,x,x);
      }  
     else {
	  sys_vgui(".x%x.c coords %xI %d %d\n",
		glist_getcanvas(glist),x,xpos + x->x_width/2 , ypos + x->x_height/2);
     }
     draw_inlets(x, glist, firsttime, 1,1);
}

void lcdbitmap_erase(t_lcdbitmap* x,t_glist* glist)
{
     int n,i,j;
    t_canvas *canvas=glist_getcanvas(glist);

	 sys_vgui(".x%x.c delete %xS\n",canvas, x);

	 sys_vgui(".x%x.c delete %xI\n",canvas,x);

     n = 1;
     while (n--) {
	  sys_vgui(".x%x.c delete %xi%d\n",canvas,x,n);
     }
     n = 1;
     while (n--) {
	  sys_vgui(".x%x.c delete %xo%d\n",canvas,x,n);
     }
}

/* ------------------------ lcdbitmap widgetbehaviour----------------------------- */


static void lcdbitmap_getrect(t_gobj *z, t_glist *glist,
    int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_lcdbitmap *x = (t_lcdbitmap *)z;
    int width, height;
    t_lcdbitmap* s = (t_lcdbitmap*)z;


    width = s->x_width;
    height = s->x_height;
    *xp1 = text_xpix(&x->x_obj, glist);
    *yp1 = text_ypix(&x->x_obj, glist);
    *xp2 = *xp1 + width;
    *yp2 = *yp1 + height;
}

static void lcdbitmap_displace(t_gobj *z, t_glist *glist,
    int dx, int dy)
{
    t_lcdbitmap *x = (t_lcdbitmap *)z;
    x->x_obj.te_xpos += dx;
    x->x_obj.te_ypos += dy;
    lcdbitmap_drawme(x, glist, 0);
    canvas_fixlinesfor(glist_getcanvas(glist),(t_text*) x);
}

static void lcdbitmap_select(t_gobj *z, t_glist *glist, int state)
{
     t_lcdbitmap *x = (t_lcdbitmap *)z;
    /* sys_vgui(".x%x.c itemconfigure %xS -fill %s\n", glist, 
	     x, (state? "blue" : BACKGROUNDCOLOR));*/
}


static void lcdbitmap_activate(t_gobj *z, t_glist *glist, int state)
{
/*    t_text *x = (t_text *)z;
    t_rtext *y = glist_findrtext(glist, x);
    if (z->g_pd != gatom_class) rtext_activate(y, state);*/
}

static void lcdbitmap_delete(t_gobj *z, t_glist *glist)
{
    t_text *x = (t_text *)z;
    canvas_deletelinesfor(glist, x);
}

       
static void lcdbitmap_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_lcdbitmap* s = (t_lcdbitmap*)z;
    if (vis)
	 lcdbitmap_drawme(s, glist, 1);
    else
	 lcdbitmap_erase(s,glist);
}


static void lcdbitmap_save(t_gobj *z, t_binbuf *b)
{
    t_lcdbitmap *x = (t_lcdbitmap *)z;
	 
    binbuf_addv(b, "ssiisii", gensym("#X"),gensym("obj"),
		(t_int)x->x_obj.te_xpos, (t_int)x->x_obj.te_ypos,  
		gensym("lcdbitmap"),x->x_width,x->x_height);
    binbuf_addv(b, ";");
}

void lcdbitmap_draw_byte(t_lcdbitmap *x,int i,int line)
{
	t_canvas *canvas=glist_getcanvas(x->x_glist);
	char *color;
	int j;
	
	/*if(glist_isvisible(x->x_glist)) for(j=0;j<8;j++){
		color=((x->x_bitmap[i][line])&(1<<j))?BLACKCOLOR:WHITECOLOR;
		sys_vgui(".x%x.c itemconfigure %x%d_%d -fill %s -outline %s\n", canvas, 
			x, i,j,color,color);		
	}*/
}

void lcdbitmap_bang(t_lcdbitmap *x)
{
	int scale=1;
	int X=ceil((122*scale)/8.0);
	int Y=32*scale;
	int xs,ys,xx,u,v,i,j,jj,tmp;
	char *s=x->x_bitstr;
	// xs[xx],ys:bitstr  u,v:real_bits  i,j[jj]:lcd 
	
	s+=sprintf(s,"\
#define BM_width 122\\n\
#define BM_height 32\\n\
static unsigned char BM_bits[] = {\n");
	
	for(ys=0;ys<Y;ys++) {
		for(xs=0;xs<X;xs++) { /*x,y = pos dans bmp*/
			tmp=0;
			for(xx=0;xx<8;xx++){
				u=(xs*8+xx)/scale;	
				v=ys/scale;
				
				i=u;
				j=v/8;
				jj=v%8;
				
				if( i < 122 )
				tmp+= (((x->x_bitmap[i][j]>>jj) & 1) << /*(7-xx)*/xx) ;
			}
			//bytes_lcd[i]=tmp;
			s+=sprintf(s," 0x%.2x,",tmp);
		}
		//s+=sprintf(s,"\n");
	}
	s+=sprintf(s," };");
	
	if(glist_isvisible(x->x_glist)) {
	 	//sys_vgui("img%x blank\n",x);
		//sys_vgui("img%x configure -data \n",x);
		sys_vgui("img%x configure -data \"%s\"\n",x,x->x_bitstr);
	 	//post("img%x configure -data \"%s\"\n",x,x->x_bitstr);
		sys_vgui(".x%x.c itemconfigure %xI -image imgnull\n",
			glist_getcanvas(x->x_glist),x);	
		sys_vgui(".x%x.c itemconfigure %xI -image img%x\n",
			glist_getcanvas(x->x_glist),x,x);	
	}
}	

void lcdbitmap_float(t_lcdbitmap *x,t_floatarg fbyte)
{
	int i,j=x->x_page,byte=(int)fbyte;
		
	if(x->x_x>61) return;
	
	i=x->x_x+61*x->x_side;
	x->x_bitmap[i][j]=byte;
	//lcdbitmap_draw_byte(x,i,j);
	x->x_x=x->x_x+1;
}

void lcdbitmap_locate(t_lcdbitmap *x,t_floatarg side,t_floatarg page,t_floatarg column)
{
	if( (side<0)||(side>1)) post("lcdbitmap_locate ERROR: side=%d",side);
	else x->x_side=(int)side;

	if( (page<0)||(page>3)) post("lcdbitmap_locate ERROR: page=%d",page);
	else x->x_page=(int)page;

	if( (column<0)||(column>61)) post("lcdbitmap_locate ERROR: column=%d",column);
	else x->x_x=(int)column;
}


static void lcdbitmap_click(t_lcdbitmap *x, t_floatarg xpos, t_floatarg ypos,
			  t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
/*	int note;    
    int x0=text_xpix(&x->x_obj, x->x_glist);
    int y0=text_ypix(&x->x_obj, x->x_glist);
	
	note=get_touched_note(
		(xpos-x0)/x->x_width,
		(ypos-y0)/x->x_height);
			
	if(note>=0) lcdbitmap_set(x,note,!x->x_notes[note]);*/
}

static int lcdbitmap_newclick(t_gobj *z, struct _glist *glist,
			    int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_lcdbitmap* x = (t_lcdbitmap *)z;

    if(doit)
    {
	lcdbitmap_click( x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
		0, (t_floatarg)alt);
    }
    return (1);
}


void lcdbitmap_size(t_lcdbitmap* x,t_floatarg w,t_floatarg h) {
     x->x_width = w;
     x->x_height = h;
     if(glist_isvisible(x->x_glist)) lcdbitmap_drawme(x, x->x_glist, 0);
}

t_widgetbehavior   lcdbitmap_widgetbehavior;

static void lcdbitmap_setwidget(void)
{
    lcdbitmap_widgetbehavior.w_getrectfn =	lcdbitmap_getrect;
    lcdbitmap_widgetbehavior.w_displacefn =	lcdbitmap_displace;
    lcdbitmap_widgetbehavior.w_selectfn =	lcdbitmap_select;
    lcdbitmap_widgetbehavior.w_activatefn =	lcdbitmap_activate;
    lcdbitmap_widgetbehavior.w_deletefn =	lcdbitmap_delete;
    lcdbitmap_widgetbehavior.w_visfn =		lcdbitmap_vis;
    lcdbitmap_widgetbehavior.w_clickfn =	lcdbitmap_newclick;
    //lcdbitmap_widgetbehavior.w_propertiesfn =	NULL; 
    //lcdbitmap_widgetbehavior.w_savefn =			lcdbitmap_save;
}


static void *lcdbitmap_new(t_symbol *s, int argc, t_atom *argv)
{
	int i,j,err=0;
	
	t_lcdbitmap *x = (t_lcdbitmap *)pd_new(lcdbitmap_class);

	x->x_glist = (t_glist*) canvas_getcurrent();
	x->x_width = DEFAULTWIDTH;
	x->x_height = DEFAULTHEIGHT;
	//outlet_new(&x->x_obj, &s_float);
	x->x_side=0;
	x->x_page=0;
	x->x_x=0;
	
	for(j=0;j<32;j++) for(i=0;i<122;i++) x->x_bitmap[i][j]=0;
	//x->x_bitstr=strdup(Hurlo);
	strcpy(x->x_bitstr,Hurlo);

	if((argc>1)&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1))
	{
		if(atom_getfloat(&argv[0])) x->x_width = atom_getfloat(&argv[0]);
		if(atom_getfloat(&argv[1])) x->x_height = atom_getfloat(&argv[1]);
	}

	return (x);
}

void lcdbitmap_setup(void)
{
    post("lcdbitmap_setup");
	 lcdbitmap_class = class_new(gensym("lcdbitmap"), (t_newmethod)lcdbitmap_new, 0,
				sizeof(t_lcdbitmap),0, A_GIMME,0);

    class_addfloat(lcdbitmap_class,lcdbitmap_float);
    
    class_addbang(lcdbitmap_class,lcdbitmap_bang);

    class_addmethod(lcdbitmap_class, (t_method)lcdbitmap_locate, gensym("locate"),
    	A_FLOAT, A_FLOAT, A_FLOAT,0);

    class_addmethod(lcdbitmap_class, (t_method)lcdbitmap_click, gensym("click"),
    	A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);

    class_addmethod(lcdbitmap_class, (t_method)lcdbitmap_size, gensym("size"),
    	A_FLOAT, A_FLOAT, 0);


    lcdbitmap_setwidget();
    class_setwidget(lcdbitmap_class,&lcdbitmap_widgetbehavior);
    //class_setsavefn(lcdbitmap_class, lcdbitmap_save);
	class_sethelpsymbol(lcdbitmap_class, gensym("moonlibs/lcdbitmap"));
}


