/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* mknob.c written by Antoine Rousseau from g_vslider.c.*/
/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */


#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"
#include "g_canvas.h"

#include "g_all_guis.h"
#include <math.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

#include <dlfcn.h>

#define MKNOB_TANGLE 100
#define MKNOB_DEFAULTH 100
#define MKNOB_DEFAULTSIZE 25
#define MKNOB_MINSIZE 12
/* ------------ mknob  ----------------------- */
typedef struct _mknob
{
    t_iemgui x_gui;
    int      x_pos;
    int      x_val;
    int      x_lin0_log1;
    int      x_steady;
    double   x_min;
    double   x_max;
    int   	 x_H;
    double   x_k;
    t_float  x_fval;
} t_mknob;

t_widgetbehavior mknob_widgetbehavior;
static t_class *mknob_class;

/* widget helper functions */

static void mknob_draw_io(t_mknob *x,t_glist *glist, int old_snd_rcv_flags);
static void mknob_draw_config(t_mknob *x,t_glist *glist);

static void mknob_update_knob(t_mknob *x, t_glist *glist)
{
    t_canvas *canvas = glist_getcanvas(glist);
    float val = (x->x_val + 50.0) / 100.0 / x->x_H;
    float angle;
    float radius = x->x_gui.x_w / 2.0;
    float miniradius = radius / 6.0;
    int x0, y0, x1, y1, xc, yc, xp, yp, xpc, ypc;
    char tag[128];

    if(miniradius < 3.0) miniradius = 3.0;

    x0 = text_xpix(&x->x_gui.x_obj, glist);
    y0 = text_ypix(&x->x_gui.x_obj, glist);
    x1 = x0 + x->x_gui.x_w;
    y1 = y0 + x->x_gui.x_w;
    xc = (x0 + x1) / 2;
    yc = (y0 + y1) / 2;

    if(x->x_gui.x_h<0)
        angle=val*(M_PI*2)+M_PI/2.0;
    else
        angle=val*(M_PI*1.5)+3.0*M_PI/4.0;

    xp=xc+radius*cos(angle);
    yp=yc+radius*sin(angle);
    xpc=miniradius*cos(angle-M_PI/2);
    ypc=miniradius*sin(angle-M_PI/2);

    sprintf(tag, "%pWIPER", x);
    pdgui_vmess(0, "crs iiiiii", canvas, "coords", tag,
            xp, yp, xc+xpc, yc+ypc, xc-xpc, yc-ypc);
}

static void mknob_draw_update(t_mknob *x, t_glist *glist)
{
    if (glist_isvisible(glist))
    {
        mknob_update_knob(x,glist);
    }
}

static void mknob_draw_new(t_mknob *x, t_glist *glist)
{
    char tag[128], tag_object[128], tag_select[128];
    char *tags[] = {tag_object, tag, "label", "text"};
    char *seltags[] = {tag_object, tag, tag_select};
    t_canvas *canvas=glist_getcanvas(glist);

    sprintf(tag_object, "%pOBJ", x);
    sprintf(tag_select, "%pSELECT", x);

    sprintf(tag, "%pBASE", x);
    pdgui_vmess(0, "crr iiii rS", canvas, "create", "oval",
         0, 0, 0, 0, "-tags", 3, seltags);

    mknob_draw_io(x, glist, 0);

    sprintf(tag, "%pWIPER", x);
    pdgui_vmess(0, "crr iiiiii rS", canvas, "create", "polygon",
         0, 0, 0, 0, 0, 0, "-tags", 2, tags);

    sprintf(tag, "%pLABEL", x);
    pdgui_vmess(0, "crr ii rs rS", canvas, "create", "text",
         0, 0,
         "-anchor", "w",
         "-tags", 4, tags);

     mknob_draw_config(x, glist);
}

static void mknob_draw_config(t_mknob *x,t_glist *glist)
{
    t_canvas *canvas=glist_getcanvas(glist);
    int xpos = text_xpix(&x->x_gui.x_obj, glist);
    int ypos = text_ypix(&x->x_gui.x_obj, glist);
    char tag[128];
    const int zoom = IEMGUI_ZOOM(x);
    t_iemgui *iemgui = &x->x_gui;
    t_atom fontatoms[3];
    SETSYMBOL(fontatoms+0, gensym(iemgui->x_font));
    SETFLOAT (fontatoms+1, -iemgui->x_fontsize*zoom);
    SETSYMBOL(fontatoms+2, gensym(sys_fontweight));

    sprintf(tag, "%pLABEL", x);
    pdgui_vmess(0, "crs ii", canvas, "coords", tag,
        xpos+x->x_gui.x_ldx * zoom,
        ypos+x->x_gui.x_ldy * zoom);

    pdgui_vmess(0, "crs rA rk", canvas, "itemconfigure", tag,
        "-font", 3, fontatoms,
        "-fill", (x->x_gui.x_fsf.x_selected ? IEM_GUI_COLOR_SELECTED : x->x_gui.x_lcol));

    sprintf(tag, "%pWIPER", x);
    pdgui_vmess(0, "crs rk ri", canvas, "itemconfigure", tag,
        "-fill", x->x_gui.x_fcol,
        "-width", 3 * zoom);

    sprintf(tag, "%pBASE", x);
    pdgui_vmess(0, "crs rk", canvas, "itemconfigure", tag,
        "-fill", x->x_gui.x_bcol);

    pdgui_vmess(0, "crs iiii", canvas, "coords", tag,
        xpos, ypos,
        xpos + x->x_gui.x_w, ypos + x->x_gui.x_w);

    mknob_update_knob(x, glist);
    iemgui_dolabel(x, &x->x_gui, x->x_gui.x_lab, 1);
}

static void mknob_draw_io(t_mknob *x,t_glist *glist, int old_snd_rcv_flags)
{
    int xpos=text_xpix(&x->x_gui.x_obj, glist);
    int ypos=text_ypix(&x->x_gui.x_obj, glist);
    int iow = IOWIDTH * IEMGUI_ZOOM(x), ioh = IEM_GUI_IOHEIGHT * IEMGUI_ZOOM(x);
    t_canvas *canvas=glist_getcanvas(glist);
    char tag_object[128], tag[128], tag_select[128], tag_label[128];
    char *tags[] = {tag_object, tag, tag_select};
    int show_io = (!x->x_gui.x_fsf.x_snd_able) || (!x->x_gui.x_fsf.x_rcv_able);

    sprintf(tag_object, "%pOBJ", x);
    sprintf(tag_select, "%pSELECT", x);
    sprintf(tag_label, "%pLABEL", x);

    sprintf(tag, "%pOUTLINE", x);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if (show_io)
    {
        pdgui_vmess(0, "crr iiii ri rS", canvas, "create", "rectangle",
            xpos, ypos,
            xpos + x->x_gui.x_w, ypos + x->x_gui.x_w,
            "-width", IEMGUI_ZOOM(x),
            "-tags", 3, tags);
    }

    sprintf(tag, "%pOUT%d", x, 0);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if (show_io)
    {
        pdgui_vmess(0, "crr iiii rs rS", canvas, "create", "rectangle",
            xpos, ypos + x->x_gui.x_w + IEMGUI_ZOOM(x) - ioh,
            xpos + iow, ypos + x->x_gui.x_w,
            "-fill", "black",
            "-tags", 2, tags);

            /* keep label above outlet */
        pdgui_vmess(0, "crss", canvas, "raise", tag_label, tag);
    }

    sprintf(tag, "%pIN%d", x, 0);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if (show_io)
    {
        pdgui_vmess(0, "crr iiii rs rS", canvas, "create", "rectangle",
            xpos, ypos,
            xpos + iow, ypos - IEMGUI_ZOOM(x) + ioh,
            "-fill", "black",
            "-tags", 2, tags);

            /* keep label above inlet */
        pdgui_vmess(0, "crss", canvas, "raise", tag_label, tag);
    }
}

static void mknob_draw_select(t_mknob *x,t_glist *glist)
{
    t_canvas *canvas = glist_getcanvas(glist);
    int lcol = x->x_gui.x_lcol;
    int col = IEM_GUI_COLOR_NORMAL;
    char tag[128];

    if(x->x_gui.x_fsf.x_selected)
        lcol = col = IEM_GUI_COLOR_SELECTED;

    sprintf(tag, "%pSELECT", x);
    pdgui_vmess(0, "crs rk", canvas, "itemconfigure", tag, "-outline", col);
    sprintf(tag, "%pLABEL", x);
    pdgui_vmess(0, "crs rk", canvas, "itemconfigure", tag, "-fill", lcol);

}

/* ------------------------ mknob widgetbehaviour----------------------------- */

#define GRECTRATIO 0
static void mknob_getrect(t_gobj *z, t_glist *glist,
                          int *xp1, int *yp1, int *xp2, int *yp2)
{
    t_mknob *x = (t_mknob *)z;

    *xp1 = text_xpix(&x->x_gui.x_obj, glist) + GRECTRATIO * x->x_gui.x_w;
    *yp1 = text_ypix(&x->x_gui.x_obj, glist) + GRECTRATIO * x->x_gui.x_w;
    *xp2 = text_xpix(&x->x_gui.x_obj, glist) + x->x_gui.x_w * (1 - GRECTRATIO);
    *yp2 = text_ypix(&x->x_gui.x_obj, glist) + x->x_gui.x_w * (1 - GRECTRATIO);
}

static void mknob_save(t_gobj *z, t_binbuf *b)
{
    t_mknob *x = (t_mknob *)z;
    t_symbol *bflcol[3];
    t_symbol *srl[3];

    ((void (*)(t_iemgui *iemgui, t_symbol **srl, t_symbol **bflcol))&iemgui_save)(&x->x_gui, srl, bflcol);
    binbuf_addv(b, "ssiisiiffiisssiiiisssii", gensym("#X"),gensym("obj"),
        (t_int)x->x_gui.x_obj.te_xpix, (t_int)x->x_gui.x_obj.te_ypix,
        atom_getsymbol(binbuf_getvec(x->x_gui.x_obj.te_binbuf)),
        x->x_gui.x_w, x->x_gui.x_h,
        (float)x->x_min, (float)x->x_max,
        x->x_lin0_log1, iem_symargstoint(&x->x_gui.x_isa),
        srl[0], srl[1], srl[2],
        x->x_gui.x_ldx, x->x_gui.x_ldy,
        iem_fstyletoint(&x->x_gui.x_fsf), x->x_gui.x_fontsize,
        bflcol[0], bflcol[1], bflcol[2],
        x->x_val, x->x_steady);
    binbuf_addv(b, ";");
}

void mknob_update_H(t_mknob *x)
{
    int H;

    H = x->x_gui.x_h;
    if(H < 0) H = 360;
    if(H == 0) H = 270;
    x->x_H = H;

    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_H/IEMGUI_ZOOM(x) - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_H/IEMGUI_ZOOM(x) - 1);
}

void mknob_check_wh(t_mknob *x, int w, int h)
{
    int H;

    if(w < MKNOB_MINSIZE) w = MKNOB_MINSIZE;
    x->x_gui.x_w = w * IEMGUI_ZOOM(x);

    if(h < -1) h = -1;
    if((h > 0) && (h < 20)) h = 20;
    x->x_gui.x_h = h * IEMGUI_ZOOM(x);

    mknob_update_H(x);
}

void mknob_check_minmax(t_mknob *x, double min, double max)
{
    int H;

    if(x->x_lin0_log1)
    {
        if((min == 0.0)&&(max == 0.0))
            max = 1.0;
        if(max > 0.0)
        {
            if(min <= 0.0)
                min = 0.01*max;
        }
        else
        {
            if(min > 0.0)
                max = 0.01*min;
        }
    }
    x->x_min = min;
    x->x_max = max;

    if(x->x_lin0_log1)
        x->x_k = log(x->x_max/x->x_min)/(double)(x->x_H/IEMGUI_ZOOM(x) - 1);
    else
        x->x_k = (x->x_max - x->x_min)/(double)(x->x_H/IEMGUI_ZOOM(x) - 1);
}

static void mknob_properties(t_gobj *z, t_glist *owner)
{
    t_mknob *x = (t_mknob *)z;
    char buf[800];
    t_symbol *srl[3];

    iemgui_properties(&x->x_gui, srl);
    sprintf(buf, "pdtk_iemgui_dialog %%s mknob \
        --------dimension(pix):-------- %d %d size: %d %d mouse: \
        -----------output-range:----------- %g left: %g right: %g \
        %d lin log %d %d empty %d \
        {%s} {%s} \
        {%s} %d %d \
        %d %d \
        #%06x #%06x #%06x\n",
            x->x_gui.x_w/IEMGUI_ZOOM(x), MKNOB_MINSIZE, x->x_gui.x_h/IEMGUI_ZOOM(x), -1,
            x->x_min, x->x_max, 0.0,/*no_schedule*/
            x->x_lin0_log1, x->x_gui.x_isa.x_loadinit, x->x_steady, -1,/*no multi, but iem-characteristic*/
            srl[0]?srl[0]->s_name:"", srl[1]?srl[1]->s_name:"",
            srl[2]?srl[2]->s_name:"", x->x_gui.x_ldx, x->x_gui.x_ldy,
            x->x_gui.x_fsf.x_font_style, x->x_gui.x_fontsize,
            0xffffff & x->x_gui.x_bcol, 0xffffff & x->x_gui.x_fcol, 0xffffff & x->x_gui.x_lcol);
    gfxstub_new(&x->x_gui.x_obj.ob_pd, x, buf);
}

    /* compute numeric value (fval) from pixel location (val) and range */
static t_float mknob_getfval(t_mknob *x)
{
    t_float fval;
    int zoom = x->x_gui.x_h > 0 ? IEMGUI_ZOOM(x) : 1;
    int zoomval = (x->x_gui.x_fsf.x_finemoved) ?
        x->x_val/zoom : (x->x_val / (100*zoom)) * 100;

    if (x->x_lin0_log1)
        fval = x->x_min * exp(x->x_k * (double)(zoomval) * 0.01);
    else fval = (double)(zoomval) * 0.01 * x->x_k + x->x_min;
    if ((fval < 1.0e-10) && (fval > -1.0e-10))
        fval = 0.0;
    return (fval);
}

static void mknob_set(t_mknob *x, t_floatarg f)
{
    int old = x->x_val;
    double g;

    x->x_fval = f;
    if (x->x_min > x->x_max)
    {
        if(f > x->x_min)
            f = x->x_min;
        if(f < x->x_max)
            f = x->x_max;
    }
    else
    {
        if(f > x->x_max)
            f = x->x_max;
        if(f < x->x_min)
            f = x->x_min;
    }
    if(x->x_lin0_log1)
        g = log(f/x->x_min)/x->x_k;
    else
        g = (f - x->x_min) / x->x_k;
    x->x_val = (x->x_gui.x_h > 0 ? IEMGUI_ZOOM(x) : 1) * (int)(100.0*g + 0.49999);
    x->x_pos = x->x_val;
    if(x->x_val != old)
    	(*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
}

static void mknob_bang(t_mknob *x)
{
    double out;

    if (pd_compatibilitylevel < 46)
        out = mknob_getfval(x);
    else out = x->x_fval;
    outlet_float(x->x_gui.x_obj.ob_outlet, out);
    if(x->x_gui.x_fsf.x_snd_able && x->x_gui.x_snd && x->x_gui.x_snd->s_thing)
        pd_float(x->x_gui.x_snd->s_thing, out);
}

static void mknob_dialog(t_mknob *x, t_symbol *s, int argc, t_atom *argv)
{
    t_symbol *srl[3];
    int w = (int)atom_getintarg(0, argc, argv);
    int h = (int)atom_getintarg(1, argc, argv);
    double min = (double)atom_getfloatarg(2, argc, argv);
    double max = (double)atom_getfloatarg(3, argc, argv);
    int lilo = (int)atom_getintarg(4, argc, argv);
    int steady = (int)atom_getintarg(17, argc, argv);
    int sr_flags;

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;
    if(steady)
        x->x_steady = 1;
    else
        x->x_steady = 0;
    sr_flags = iemgui_dialog(&x->x_gui, srl, argc, argv);

    mknob_check_wh(x, w, h);
    mknob_check_minmax(x, min, max);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_CONFIG);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_IO + sr_flags);
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    canvas_fixlinesfor(x->x_gui.x_glist, (t_text *)x);
    mknob_set(x, x->x_fval);
}

static int xm0,ym0,xm,ym;

static void mknob_motion(t_mknob *x, t_floatarg dx, t_floatarg dy, t_floatarg unused)
{
    int old = x->x_val;
    float d=-dy;

    if (abs((int)dx)>abs((int)dy)) d=dx;

    if(x->x_gui.x_fsf.x_finemoved)
        x->x_pos += (int)d;
    else
        x->x_pos += 100*(int)d;
    x->x_val = x->x_pos;
    if(x->x_val > (100*x->x_H - 100))
    {
        x->x_val = 100*x->x_H - 100;
        x->x_pos += 50;
        x->x_pos -= x->x_pos%100;
    }
    if(x->x_val < 0)
    {
        x->x_val = 0;
        x->x_pos -= 50;
        x->x_pos -= x->x_pos%100;
    }
    x->x_fval = mknob_getfval(x);
    if(old != x->x_val)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        mknob_bang(x);
    }
}

static void mknob_motion_circular(t_mknob *x, t_floatarg dx, t_floatarg dy, t_floatarg unused)
{
    int xc=text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist)+x->x_gui.x_w/2;
    int yc=text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist)+x->x_gui.x_w/2;
    int old = x->x_val;
    float alpha;

    xm+=dx;
    ym+=dy;

    alpha=atan2(xm-xc,ym-yc)*180.0/M_PI;

    x->x_pos=(int)(31500-alpha*100.0)%36000;
    if(x->x_pos>31500) x->x_pos=0;
    else if(x->x_pos>(27000-100)) x->x_pos=(27000-100);

    x->x_val=x->x_pos;
    x->x_fval = mknob_getfval(x);

    if(old != x->x_val)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        mknob_bang(x);
    }
}

static void mknob_motion_fullcircular(t_mknob *x, t_floatarg dx, t_floatarg dy, t_floatarg unused)
{
    int xc=text_xpix(&x->x_gui.x_obj, x->x_gui.x_glist)+x->x_gui.x_w/2;
    int yc=text_ypix(&x->x_gui.x_obj, x->x_gui.x_glist)+x->x_gui.x_w/2;
    int old = x->x_val;
    float alpha;

    xm+=dx;
    ym+=dy;

    alpha=atan2(xm-xc,ym-yc)*180.0/M_PI;

    x->x_pos=(int)(36000-alpha*100.0)%36000;

    if(x->x_pos>(36000-100)) x->x_pos=(36000-100);
    x->x_val=x->x_pos;
    x->x_fval = mknob_getfval(x);

    if(old != x->x_val)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        mknob_bang(x);
    }
}

static void mknob_click(t_mknob *x, t_floatarg xpos, t_floatarg ypos,
                        t_floatarg shift, t_floatarg ctrl, t_floatarg alt)
{
    xm0=xm=xpos;
    ym0=ym=ypos;
    if(x->x_val > (100*x->x_H - 100))
        x->x_val = 100*x->x_H - 100;
    if(x->x_val < 0)
        x->x_val = 0;
    x->x_fval = mknob_getfval(x);
    x->x_pos = x->x_val;
    (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
    mknob_bang(x);

    if(x->x_gui.x_h<0)
        glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
                   (t_glistmotionfn)mknob_motion_fullcircular, 0, xpos, ypos);
    else if(x->x_gui.x_h==0)
        glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
                   (t_glistmotionfn)mknob_motion_circular, 0, xpos, ypos);
    else
        glist_grab(x->x_gui.x_glist, &x->x_gui.x_obj.te_g,
                   (t_glistmotionfn)mknob_motion, 0, xpos, ypos);

}

static int mknob_newclick(t_gobj *z, struct _glist *glist,
                          int xpix, int ypix, int shift, int alt, int dbl, int doit)
{
    t_mknob *x = (t_mknob *)z;

    if(doit)
    {
        mknob_click( x, (t_floatarg)xpix, (t_floatarg)ypix, (t_floatarg)shift,
                     0, (t_floatarg)alt);
        if(shift)
            x->x_gui.x_fsf.x_finemoved = 1;
        else
            x->x_gui.x_fsf.x_finemoved = 0;
    }
    return (1);
}

static void mknob_size(t_mknob *x, t_symbol *s, int ac, t_atom *av)
{
    int w = (int)atom_getintarg(0, ac, av);
    int h = x->x_gui.x_h;

    if(ac > 1) h= (int)atom_getintarg(1, ac, av);

    mknob_check_wh(x, w, h);
    iemgui_size((void *)x, &x->x_gui);
    if(glist_isvisible(x->x_gui.x_glist)) 
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_MOVE);
    mknob_set(x, x->x_fval);
}

static void mknob_delta(t_mknob *x, t_symbol *s, int ac, t_atom *av)
{
    iemgui_delta((void *)x, &x->x_gui, s, ac, av);
}

static void mknob_pos(t_mknob *x, t_symbol *s, int ac, t_atom *av)
{
    iemgui_pos((void *)x, &x->x_gui, s, ac, av);
}

static void mknob_range(t_mknob *x, t_symbol *s, int ac, t_atom *av)
{
    mknob_check_minmax(x, (double)atom_getfloatarg(0, ac, av),
                       (double)atom_getfloatarg(1, ac, av));
    mknob_set(x, x->x_fval);
}

static void mknob_color(t_mknob *x, t_symbol *s, int ac, t_atom *av)
{
    iemgui_color((void *)x, &x->x_gui, s, ac, av);
}

static void mknob_send(t_mknob *x, t_symbol *s)
{
    iemgui_send(x, &x->x_gui, s);
}

static void mknob_receive(t_mknob *x, t_symbol *s)
{
    iemgui_receive(x, &x->x_gui, s);
}

static void mknob_label(t_mknob *x, t_symbol *s)
{
    iemgui_label((void *)x, &x->x_gui, s);
}

static void mknob_label_pos(t_mknob *x, t_symbol *s, int ac, t_atom *av)
{
    iemgui_label_pos((void *)x, &x->x_gui, s, ac, av);
}

static void mknob_label_font(t_mknob *x, t_symbol *s, int ac, t_atom *av)
{
    iemgui_label_font((void *)x, &x->x_gui, s, ac, av);
}

static void mknob_log(t_mknob *x)
{
    x->x_lin0_log1 = 1;
    mknob_check_minmax(x, x->x_min, x->x_max);
    mknob_set(x, x->x_fval);
}

static void mknob_lin(t_mknob *x)
{
    x->x_lin0_log1 = 0;
    x->x_k = (x->x_max - x->x_min)/(double)(x->x_gui.x_w/IEMGUI_ZOOM(x) - 1);
    mknob_set(x, x->x_fval);
}

static void mknob_init(t_mknob *x, t_floatarg f)
{
    x->x_gui.x_isa.x_loadinit = (f==0.0)?0:1;
}

static void mknob_steady(t_mknob *x, t_floatarg f)
{
    x->x_steady = (f==0.0)?0:1;
}

static void mknob_float(t_mknob *x, t_floatarg f)
{
    double out;

    mknob_set(x, f);
    if(x->x_gui.x_fsf.x_put_in2out)
        mknob_bang(x);
}

static void mknob_zoom(t_mknob *x, t_floatarg f)
{
    if(x->x_gui.x_h > 0) 
    {
    /* scale current pixel value */
        x->x_val = (IEMGUI_ZOOM(x) == 2 ? (x->x_val)/2 : (x->x_val)*2);
        x->x_H = (IEMGUI_ZOOM(x) == 2 ? (x->x_H)/2 : (x->x_H)*2);
        x->x_pos = x->x_val;
    }
    iemgui_zoom(&x->x_gui, f);
}

static void mknob_loadbang(t_mknob *x, t_floatarg action)
{
    if (action == LB_LOAD && x->x_gui.x_isa.x_loadinit)
    {
        (*x->x_gui.x_draw)(x, x->x_gui.x_glist, IEM_GUI_DRAW_MODE_UPDATE);
        mknob_bang(x);
    }
}

static void *mknob_new(t_symbol *s, int argc, t_atom *argv)
{
    t_mknob *x = (t_mknob *)iemgui_new(mknob_class);
    int width = MKNOB_DEFAULTSIZE, height = MKNOB_DEFAULTH;
    int fs = 8, lilo = 0, ldx = -2, ldy = -6, f = 0, v = 0, steady = 1;
    double min = 0.0, max = (double)(IEM_SL_DEFAULTSIZE-1);
    char str[144];

    iem_inttosymargs(&x->x_gui.x_isa, 0);
    iem_inttofstyle(&x->x_gui.x_fsf, 0);

    x->x_gui.x_bcol = 0xFCFCFC;
    x->x_gui.x_fcol = 0x00;
    x->x_gui.x_lcol = 0x00;

    IEMGUI_SETDRAWFUNCTIONS(x, mknob);

    if(((argc == 17)||(argc == 18))&&IS_A_FLOAT(argv,0)&&IS_A_FLOAT(argv,1)
            &&IS_A_FLOAT(argv,2)&&IS_A_FLOAT(argv,3)
            &&IS_A_FLOAT(argv,4)&&IS_A_FLOAT(argv,5)
            &&(IS_A_SYMBOL(argv,6)||IS_A_FLOAT(argv,6))
            &&(IS_A_SYMBOL(argv,7)||IS_A_FLOAT(argv,7))
            &&(IS_A_SYMBOL(argv,8)||IS_A_FLOAT(argv,8))
            &&IS_A_FLOAT(argv,9)&&IS_A_FLOAT(argv,10)
            &&IS_A_FLOAT(argv,11)&&IS_A_FLOAT(argv,12)&&IS_A_FLOAT(argv,16))
    {
        width = (int)atom_getintarg(0, argc, argv);
        height = (int)atom_getintarg(1, argc, argv);
        min = (double)atom_getfloatarg(2, argc, argv);
        max = (double)atom_getfloatarg(3, argc, argv);
        lilo = (int)atom_getintarg(4, argc, argv);
        iem_inttosymargs(&x->x_gui.x_isa, atom_getintarg(5, argc, argv));
        iemgui_new_getnames(&x->x_gui, 6, argv);
        ldx = (int)atom_getintarg(9, argc, argv);
        ldy = (int)atom_getintarg(10, argc, argv);
        iem_inttofstyle(&x->x_gui.x_fsf, atom_getintarg(11, argc, argv));
        fs = (int)atom_getintarg(12, argc, argv);

        iemgui_all_loadcolors(&x->x_gui, argv+13, argv+14, argv+15);

        v = (int)atom_getintarg(16, argc, argv);
    }
    else iemgui_new_getnames(&x->x_gui, 6, 0);

    if((argc == 18)&&IS_A_FLOAT(argv,17))
        steady = (int)atom_getintarg(17, argc, argv);

    x->x_gui.x_glist = (t_glist *)canvas_getcurrent();

    if(x->x_gui.x_isa.x_loadinit)
        x->x_val = v;
    else
        x->x_val = 0;

    x->x_pos = x->x_val;

    if(lilo != 0) lilo = 1;
    x->x_lin0_log1 = lilo;

    if(steady != 0) steady = 1;
    x->x_steady = steady;

    x->x_gui.x_fsf.x_snd_able = 1;
    x->x_gui.x_fsf.x_rcv_able = 1;
    if(x->x_gui.x_snd == NULL || !strcmp(x->x_gui.x_snd->s_name, "empty")) x->x_gui.x_fsf.x_snd_able = 0;
    if(x->x_gui.x_snd == NULL || !strcmp(x->x_gui.x_rcv->s_name, "empty")) x->x_gui.x_fsf.x_rcv_able = 0;

    if(x->x_gui.x_fsf.x_font_style == 1) strcpy(x->x_gui.x_font, "helvetica");
    else if(x->x_gui.x_fsf.x_font_style == 2) strcpy(x->x_gui.x_font, "times");
    else
    {
        x->x_gui.x_fsf.x_font_style = 0;
        strcpy(x->x_gui.x_font, sys_font);
    }

    if(x->x_gui.x_fsf.x_rcv_able) pd_bind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);

    x->x_gui.x_ldx = ldx;
    x->x_gui.x_ldy = ldy;

    if(fs < 4) fs = 4;
    x->x_gui.x_fontsize = fs;

    mknob_check_wh(x, width, height);

    mknob_check_minmax(x, min, max);

    iemgui_verify_snd_ne_rcv(&x->x_gui);
    iemgui_newzoom(&x->x_gui);
    x->x_fval = mknob_getfval(x);
    outlet_new(&x->x_gui.x_obj, &s_float);
    return (x);
}

static void mknob_free(t_mknob *x)
{
    if(x->x_gui.x_fsf.x_rcv_able)
        pd_unbind(&x->x_gui.x_obj.ob_pd, x->x_gui.x_rcv);
    gfxstub_deleteforkey(x);
}

extern void canvas_iemguis(t_glist *gl, t_symbol *guiobjname);

void canvas_mknob(t_glist *gl, t_symbol *s, int argc, t_atom *argv)
{
    canvas_iemguis(gl, gensym("mknob"));
}

void mknob_setup(void)
{
    mknob_class = class_new(gensym("mknob"), (t_newmethod)mknob_new,
                            (t_method)mknob_free, sizeof(t_mknob), 0, A_GIMME, 0);

    class_addbang(mknob_class,mknob_bang);
    class_addfloat(mknob_class,mknob_float);
    class_addmethod(mknob_class, (t_method)mknob_click, gensym("click"),
                    A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, A_FLOAT, 0);
    class_addmethod(mknob_class, (t_method)mknob_motion, gensym("motion"),
                    A_FLOAT, A_FLOAT, 0);
    class_addmethod(mknob_class, (t_method)mknob_dialog, gensym("dialog"), A_GIMME, 0);
    class_addmethod(mknob_class, (t_method)mknob_loadbang, gensym("loadbang"), 0);
    class_addmethod(mknob_class, (t_method)mknob_set, gensym("set"), A_FLOAT, 0);
    class_addmethod(mknob_class, (t_method)mknob_size, gensym("size"), A_GIMME, 0);
    class_addmethod(mknob_class, (t_method)mknob_delta, gensym("delta"), A_GIMME, 0);
    class_addmethod(mknob_class, (t_method)mknob_pos, gensym("pos"), A_GIMME, 0);
    class_addmethod(mknob_class, (t_method)mknob_range, gensym("range"), A_GIMME, 0);
    class_addmethod(mknob_class, (t_method)mknob_color, gensym("color"), A_GIMME, 0);
    class_addmethod(mknob_class, (t_method)mknob_send, gensym("send"), A_DEFSYM, 0);
    class_addmethod(mknob_class, (t_method)mknob_receive, gensym("receive"), A_DEFSYM, 0);
    class_addmethod(mknob_class, (t_method)mknob_label, gensym("label"), A_DEFSYM, 0);
    class_addmethod(mknob_class, (t_method)mknob_label_pos, gensym("label_pos"), A_GIMME, 0);
    class_addmethod(mknob_class, (t_method)mknob_label_font, gensym("label_font"), A_GIMME, 0);
    class_addmethod(mknob_class, (t_method)mknob_log, gensym("log"), 0);
    class_addmethod(mknob_class, (t_method)mknob_lin, gensym("lin"), 0);
    class_addmethod(mknob_class, (t_method)mknob_init, gensym("init"), A_FLOAT, 0);
    class_addmethod(mknob_class, (t_method)mknob_steady, gensym("steady"), A_FLOAT, 0);
    class_addmethod(mknob_class, (t_method)mknob_zoom,
        gensym("zoom"), A_CANT, 0);

    mknob_widgetbehavior.w_getrectfn =    mknob_getrect;
    mknob_widgetbehavior.w_displacefn =   iemgui_displace;
    mknob_widgetbehavior.w_selectfn =     iemgui_select;
    mknob_widgetbehavior.w_activatefn =   NULL;
    mknob_widgetbehavior.w_deletefn =     iemgui_delete;
    mknob_widgetbehavior.w_visfn =        iemgui_vis;
    mknob_widgetbehavior.w_clickfn =      mknob_newclick;

    class_setwidget(mknob_class, &mknob_widgetbehavior);

    class_setsavefn(mknob_class, mknob_save);
    class_setpropertiesfn(mknob_class, mknob_properties);

    class_addmethod(canvas_class, (t_method)canvas_mknob, gensym("mknob"),
                    A_GIMME, A_NULL);
}
