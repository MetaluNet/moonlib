/* Copyright (c) 1997-1999 Miller Puckette.
 * For information on usage and redistribution, and for a DISCLAIMER OF ALL
 * WARRANTIES, see the file, "LICENSE.txt," in this distribution. */

/* g_7_guis.c written by Thomas Musil (c) IEM KUG Graz Austria 2000-2001 */
/* thanks to Miller Puckette, Guenther Geiger and Krzystof Czaja */


#ifndef __iemgui_static_h_

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include "m_pd.h"

#include <stdarg.h>

#ifdef _WIN32
#include <io.h>
#else
#include <unistd.h>
#endif

/* ---------------------------------------------------------------------------- */
/* import needed parts of g_all_guis.h */
#include "g_canvas.h"
#define IEM_GUI_COLOR_SELECTED       0x0000FF
#define IEM_GUI_COLOR_NORMAL         0x000000
#define IEM_GUI_COLOR_EDITED         0xFF0000

#define IEM_GUI_MAX_COLOR            30

#define IEM_GUI_DEFAULTSIZE (sys_zoomfontheight(canvas_getcurrent()->gl_font, 1, 0) + 2 + 3)
#define IEM_GUI_DEFAULTSIZE_SCALE IEM_GUI_DEFAULTSIZE/15.
#define IEM_GUI_MINSIZE     8
#define IEM_GUI_MAXSIZE     1000

#define IEM_FONT_MINSIZE    4

#define IEM_GUI_DRAW_MODE_UPDATE 0
#define IEM_GUI_DRAW_MODE_MOVE   1
#define IEM_GUI_DRAW_MODE_NEW    2
#define IEM_GUI_DRAW_MODE_SELECT 3
#define IEM_GUI_DRAW_MODE_ERASE  4
#define IEM_GUI_DRAW_MODE_CONFIG 5
#define IEM_GUI_DRAW_MODE_IO     6

#define IEM_GUI_IOHEIGHT IHEIGHT

#define IS_A_POINTER(atom,index) ((atom+index)->a_type == A_POINTER)
#define IS_A_FLOAT(atom,index) ((atom+index)->a_type == A_FLOAT)
#define IS_A_SYMBOL(atom,index) ((atom+index)->a_type == A_SYMBOL)
#define IS_A_DOLLAR(atom,index) ((atom+index)->a_type == A_DOLLAR)
#define IS_A_DOLLSYM(atom,index) ((atom+index)->a_type == A_DOLLSYM)

#define IEM_FSTYLE_FLAGS_ALL 0x007fffff
#define IEM_INIT_ARGS_ALL    0x01ffffff

#define IEM_GUI_OLD_SND_FLAG 1
#define IEM_GUI_OLD_RCV_FLAG 2

#define IEMGUI_ZOOM(x) ((x)->x_gui.x_glist->gl_zoom)

typedef struct _iem_fstyle_flags
{
    unsigned int x_font_style:6;
    unsigned int x_rcv_able:1;
    unsigned int x_snd_able:1;
    unsigned int x_lab_is_unique:1;
    unsigned int x_rcv_is_unique:1;
    unsigned int x_snd_is_unique:1;
    unsigned int x_lab_arg_tail_len:6;
    unsigned int x_lab_is_arg_num:6;
    unsigned int x_shiftdown:1;
    unsigned int x_selected:1;
    unsigned int x_finemoved:1;
    unsigned int x_put_in2out:1;
    unsigned int x_change:1;
    unsigned int x_thick:1;
    unsigned int x_lin0_log1:1;
    unsigned int x_steady:1;
} t_iem_fstyle_flags;

typedef struct _iem_init_symargs
{
    unsigned int x_loadinit:1;
    unsigned int x_rcv_arg_tail_len:6;
    unsigned int x_snd_arg_tail_len:6;
    unsigned int x_rcv_is_arg_num:6;
    unsigned int x_snd_is_arg_num:6;
    unsigned int x_scale:1;
    unsigned int x_flashed:1;
    unsigned int x_locked:1;
} t_iem_init_symargs;

typedef void (*t_iemfunptr)(void *x, t_glist *glist, int mode);

typedef void (*t_iemdrawfunptr)(void *x, t_glist *glist);
typedef struct _iemgui_drawfunctions {
    t_iemdrawfunptr draw_new; /* create all widgets */
    t_iemdrawfunptr draw_config; /* reconfigure (draw, but don't create) all widgets (except iolets) */
    t_iemfunptr     draw_iolets;  /* reconfigure (draw, but don't create) all iolets (0 uses default iolets function) */
    t_iemdrawfunptr draw_update; /* update the changeable part of the iemgui (e.g. the number in a numbox) */
    t_iemdrawfunptr draw_select; /* highlight object when it's selected */
    t_iemdrawfunptr draw_erase; /* destroy all widgets; (0 uses default erase function) */
    t_iemdrawfunptr draw_move; /* move all widgets; (0 uses default move function) */
} t_iemgui_drawfunctions;
typedef struct _iemgui
{
    t_object           x_obj;
    t_glist            *x_glist;
    t_iemfunptr        x_draw;
    int                x_h;
    int                x_w;
    struct _iemgui_private *x_private;
    int                x_ldx;
    int                x_ldy;
    char               x_font[MAXPDSTRING]; /* font names can be long! */
    t_iem_fstyle_flags x_fsf;
    int                x_fontsize;
    t_iem_init_symargs x_isa;
    unsigned int       x_fcol;
    unsigned int       x_bcol;
    unsigned int       x_lcol;
    /* send/receive/label as used ($args expanded) */
    t_symbol           *x_snd;              /* send symbol */
    t_symbol           *x_rcv;              /* receive */
    t_symbol           *x_lab;              /* label */
    /* same, with $args unexpanded */
    t_symbol           *x_snd_unexpanded;   /* NULL=uninitialized; gensym("")=empty */
    t_symbol           *x_rcv_unexpanded;
    t_symbol           *x_lab_unexpanded;
    int                x_binbufindex;       /* where in binbuf to find these */
    int                x_labelbindex;       /* where in binbuf to find label */
} t_iemgui;

/* ---------------------------------------------------------------------------- */
/* snprintf bugfix for WIN32, imported from pd/src/s_print.c for back compat */

#ifdef _WIN32

    /* NB: Unlike vsnprintf(), _vsnprintf() does *not* null-terminate
    the output if the resulting string is too large to fit into the buffer.
    Also, it just returns -1 instead of the required number of bytes.
    Strictly speaking, the UCRT in Windows 10 actually contains a standard-
    conforming vsnprintf() function that is not just an alias for _vsnprintf().
    However, MinGW traditionally links against the old msvcrt.dll runtime library.
    Recent versions of MinGW seem to have their own (standard-conformating)
    implementation of vsnprintf(), but to ensure portability we rather use our
    own implementation for all Windows builds. */
static int s_pd_vsnprintf(char *buf, size_t size, const char *fmt, va_list argptr)
{
    int ret = _vsnprintf(buf, size, fmt, argptr);
    if (ret < 0)
    {
            /* null-terminate the buffer and get the required number of bytes. */
        ret = _vscprintf(fmt, argptr);
        buf[size - 1] = '\0';
    }
    return ret;
}

static int s_pd_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = pd_vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}

#else

static int s_pd_vsnprintf(char *buf, size_t size, const char *fmt, va_list argptr)
{
    return vsnprintf(buf, size, fmt, argptr);
}

static int s_pd_snprintf(char *buf, size_t size, const char *fmt, ...)
{
    int ret;
    va_list ap;
    va_start(ap, fmt);
    ret = vsnprintf(buf, size, fmt, ap);
    va_end(ap);
    return ret;
}

#endif

/* ---------------------------------------------------------------------------- */

typedef struct _iemgui_private {
    int p_prevX, p_prevY;
    t_iemgui_drawfunctions p_widget;
    int p_binbuf_valid;
} t_iemgui_private;

typedef enum {
    FOREGROUND,
    BACKGROUND,
    LABEL,
    UNKNOWN
} colortype_t;

#define IEM_GUI_COLOR_BACKGROUND 0xFCFCFC
#define IEM_GUI_COLOR_FOREGROUND 0x000000
#define IEM_GUI_COLOR_LABEL 0x000000

/* helpers */
static int srl_is_valid(const t_symbol* s)
{
    return (!!s && s != &s_);
}

/*------------------ global variables -------------------------*/

static int iemgui_color_hex[]=
{
    16579836, 10526880, 4210752, 16572640, 16572608,
    16579784, 14220504, 14220540, 14476540, 16308476,
    14737632, 8158332, 2105376, 16525352, 16559172,
    15263784, 1370132, 2684148, 3952892, 16003312,
    12369084, 6316128, 0, 9177096, 5779456,
    7874580, 2641940, 17488, 5256, 5767248
};

/*------------------ global functions -------------------------*/

static int iemgui_clip_size(int size)
{
    if(size < IEM_GUI_MINSIZE)
        size = IEM_GUI_MINSIZE;
    return(size);
}

static int iemgui_modulo_color(int col)
{
    while(col >= IEM_GUI_MAX_COLOR)
        col -= IEM_GUI_MAX_COLOR;
    while(col < 0)
        col += IEM_GUI_MAX_COLOR;
    return(col);
}

static void iemgui_verify_snd_ne_rcv(t_iemgui *iemgui)
{
    iemgui->x_fsf.x_put_in2out = 1;
    if(iemgui->x_fsf.x_snd_able && iemgui->x_fsf.x_rcv_able)
    {
        if(!strcmp(iemgui->x_snd->s_name, iemgui->x_rcv->s_name))
            iemgui->x_fsf.x_put_in2out = 0;
    }
}

static t_symbol *iemgui_new_dogetname(t_iemgui *iemgui, int indx, t_atom *argv)
{
    if (IS_A_SYMBOL(argv, indx)) {
        t_symbol*name=atom_getsymbolarg(indx, 100000, argv);
        if(gensym("empty") == name)
            return 0;
        return name;
    }
    else if (IS_A_FLOAT(argv, indx))
    {
        char str[80];
        sprintf(str, "%d", (int)atom_getfloatarg(indx, 100000, argv));
        return (gensym(str));
    }
    else return 0;
}

static void iemgui_new_getnames(t_iemgui *iemgui, int indx, t_atom *argv)
{
    if (argv)
    {
        iemgui->x_snd = iemgui_new_dogetname(iemgui, indx, argv);
        iemgui->x_rcv = iemgui_new_dogetname(iemgui, indx+1, argv);
        if(IS_A_FLOAT(argv, indx+2))
        {
            char str[80];
            atom_string(argv+indx+2, str, sizeof(str));
            iemgui->x_lab = gensym(str);
        } else {
            iemgui->x_lab = iemgui_new_dogetname(iemgui, indx+2, argv);
        }
        iemgui->x_private->p_binbuf_valid = 1;
    }
    else
    {
        iemgui->x_snd = iemgui->x_rcv = iemgui->x_lab = 0;
        iemgui->x_private->p_binbuf_valid = 0;
    }
    /* in the object's constructor, we can't access the raw values yet: */
    iemgui->x_snd_unexpanded = iemgui->x_rcv_unexpanded = iemgui->x_lab_unexpanded = 0;
    iemgui->x_binbufindex = indx;
    iemgui->x_labelbindex = indx + 3;
}

    /* initialize a single symbol in unexpanded form.  We reach into the
    binbuf to grab them; if there's nothing there, set it to the
    fallback; if still nothing, set to NULL. */
static void iemgui_init_sym2dollararg(t_iemgui *iemgui, t_symbol **symp,
    int indx, t_symbol *fallback)
{
    t_binbuf *b = iemgui->x_obj.ob_binbuf;

    if ((!*symp) && iemgui->x_private->p_binbuf_valid && (binbuf_getnatom(b) > indx))
    {
        t_atom *a = binbuf_getvec(b) + indx;
        char astring[80];
        const char *buf = astring;
        if(A_SYMBOL == a->a_type)
        {
            buf = atom_getsymbol(a)->s_name;
        } else {
            atom_string(a, astring, sizeof(astring));
        }
        if(strcmp(buf, "empty"))
            *symp = gensym(buf);
    }
    if (!*symp)
        *symp = fallback;
}

    /* get the unexpanded versions of the symbols;
       initialize them if necessary. */
static void iemgui_all_sym2dollararg(t_iemgui *iemgui, t_symbol **srlsym)
{
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_snd_unexpanded,
        iemgui->x_binbufindex+1, iemgui->x_snd);
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_rcv_unexpanded,
        iemgui->x_binbufindex+2, iemgui->x_rcv);
    iemgui_init_sym2dollararg(iemgui, &iemgui->x_lab_unexpanded,
        iemgui->x_labelbindex, iemgui->x_lab);
    srlsym[0] = iemgui->x_snd_unexpanded;
    srlsym[1] = iemgui->x_rcv_unexpanded;
    srlsym[2] = iemgui->x_lab_unexpanded;
}

    /* helper for iemgui_all_dollararg2sym */
static t_symbol*do_all_dollarg2sym(t_iemgui*iemgui, t_symbol**s, size_t index)
{
    t_symbol*org = s[index];
    if(org) {
        s[index] = canvas_realizedollar(iemgui->x_glist, org);
    }
    return org;
}
    /* convert symbols in "$" form to the expanded symbols */
static void iemgui_all_dollararg2sym(t_iemgui *iemgui, t_symbol **srlsym)
{
        /* save unexpanded ones for later */
    iemgui->x_snd_unexpanded = do_all_dollarg2sym(iemgui, srlsym, 0);
    iemgui->x_rcv_unexpanded = do_all_dollarg2sym(iemgui, srlsym, 1);
    iemgui->x_lab_unexpanded = do_all_dollarg2sym(iemgui, srlsym, 2);
}



static t_symbol* color2symbol(unsigned int col) {
    const int  compat = (pd_compatibilitylevel < 48) ? 1 : 0;

    char colname[MAXPDSTRING];
    colname[0] = colname[MAXPDSTRING-1] = 0;

    if (compat)
    {
            /* compatibility with Pd<=0.47: saves colors as numbers with limited resolution */
        int col2 = -1 - (((0xfc0000 & col) >> 6)|((0xfc00 & col) >> 4)|((0xfc & col) >> 2));
        s_pd_snprintf(colname, MAXPDSTRING-1, "%d", col2);
    } else {
        s_pd_snprintf(colname, MAXPDSTRING-1, "#%06x", col);
    }
    return gensym(colname);
}

static void iemgui_all_col2save(t_iemgui *iemgui, t_symbol**bflcol)
{
    bflcol[0] = color2symbol(iemgui->x_bcol);
    bflcol[1] = color2symbol(iemgui->x_fcol);
    bflcol[2] = color2symbol(iemgui->x_lcol);
}

static unsigned int iemgui_getcolorarg(int index, int argc, t_atom*argv, colortype_t ct)
{
    if(index < 0 || index >= argc)
        goto default_color;
    if(IS_A_FLOAT(argv,index))
        return atom_getfloatarg(index, argc, argv);
    if(IS_A_SYMBOL(argv,index))
    {
        t_symbol*s=atom_getsymbolarg(index, argc, argv);
        if ('#' == s->s_name[0])
        {
            int col = (int)strtol(s->s_name+1, 0, 16);
            return col & 0xFFFFFF;
        }
    }

default_color:
        /* LATER use THISGUI->i_*color */
    switch(ct) {
    case FOREGROUND:
        return IEM_GUI_COLOR_FOREGROUND;
    case BACKGROUND:
        return IEM_GUI_COLOR_BACKGROUND;
    case LABEL:
        return IEM_GUI_COLOR_FOREGROUND;
    default:
        break;
    }
    return 0;
}

static unsigned int colfromatomload(t_atom*colatom, colortype_t ct)
{
    int color;
        /* old-fashioned color argument, either a number or symbol
        evaluating to an integer */
    if (colatom->a_type == A_FLOAT)
        color = atom_getfloat(colatom);
    else if (colatom->a_type == A_SYMBOL &&
        (isdigit(colatom->a_w.w_symbol->s_name[0]) ||
            colatom->a_w.w_symbol->s_name[0] == '-'))
                color = atoi(colatom->a_w.w_symbol->s_name);

        /* symbolic color */
    else return (iemgui_getcolorarg(0, 1, colatom, ct));
    if (color < 0)
    {
        color = -1 - color;
        color = ((color & 0x3f000) << 6)|((color & 0xfc0) << 4)|
            ((color & 0x3f) << 2);
    }
    else
    {
        color = iemgui_modulo_color(color);
        color = iemgui_color_hex[color];
    }
    return (color);
}

static void iemgui_all_loadcolors(t_iemgui *iemgui, t_atom*bcol, t_atom*fcol, t_atom*lcol)
{
    if(bcol)iemgui->x_bcol = colfromatomload(bcol, BACKGROUND);
    if(fcol)iemgui->x_fcol = colfromatomload(fcol, FOREGROUND);
    if(lcol)iemgui->x_lcol = colfromatomload(lcol, LABEL);
}

static unsigned int _iemgui_compatible_colorarg(int index, int argc, t_atom* argv, colortype_t ct)
{
    if (index < 0 || index >= argc)
        return 0;
    if(IS_A_FLOAT(argv,index))
        {
            int col=atom_getfloatarg(index, argc, argv);
            if(col >= 0)
            {
                int idx = iemgui_modulo_color(col);
                return(iemgui_color_hex[(idx)]);
            }
            else
               return((-1 -col)&0xffffff);
        }
    return iemgui_getcolorarg(index, argc, argv, ct);
}

static unsigned int iemgui_compatible_colorarg(int index, int argc, t_atom* argv)
{
    return _iemgui_compatible_colorarg(index, argc, argv, UNKNOWN);
}

static void iemgui_send(void *x, t_iemgui *iemgui, t_symbol *s)
{
    int sndable=1, oldsndrcvable=0;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable |= IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
        oldsndrcvable |= IEM_GUI_OLD_SND_FLAG;

    if(s && gensym("empty") == s)
        s = 0;

    if(s) {
        iemgui->x_snd_unexpanded = s;
        iemgui->x_snd = canvas_realizedollar(iemgui->x_glist, s);
    } else {
        iemgui->x_snd_unexpanded = &s_;
        iemgui->x_snd = 0;
        sndable = 0;
    }
    iemgui->x_fsf.x_snd_able = sndable;
    iemgui_verify_snd_ne_rcv(iemgui);
    if(glist_isvisible(iemgui->x_glist) && gobj_shouldvis((t_gobj *)x, iemgui->x_glist))
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_IO + oldsndrcvable);
}

static void iemgui_receive(void *x, t_iemgui *iemgui, t_symbol *s)
{
    int oldsndrcvable=0;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable |= IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
        oldsndrcvable |= IEM_GUI_OLD_SND_FLAG;

    if(s && gensym("empty") == s)
        s = 0;

    if(s) {
        iemgui->x_rcv_unexpanded = s;
        s = canvas_realizedollar(iemgui->x_glist, s);
    } else {
        iemgui->x_rcv_unexpanded = &s_;
    }
    if(s)
    {
        if(!iemgui->x_rcv || strcmp(s->s_name, iemgui->x_rcv->s_name))
        {
            if(iemgui->x_fsf.x_rcv_able)
                pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
            iemgui->x_rcv = s;
            pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        }
    }
    else if(iemgui->x_fsf.x_rcv_able)
    {
        pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
        iemgui->x_rcv = s;
    }
    iemgui->x_fsf.x_rcv_able = (s!=0);
    iemgui_verify_snd_ne_rcv(iemgui);
    if(glist_isvisible(iemgui->x_glist) && gobj_shouldvis((t_gobj *)x, iemgui->x_glist))
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_IO + oldsndrcvable);
}

static void iemgui_dolabelpos(t_object*obj, t_iemgui*iemgui) {
    int zoom = glist_getzoom(iemgui->x_glist);
    int x0 = text_xpix((t_object *)obj, iemgui->x_glist);
    int y0 = text_ypix((t_object *)obj, iemgui->x_glist);
    int dx = iemgui->x_ldx, dy = iemgui->x_ldy;
    char tag[128];
    sprintf(tag, "%p_LABEL", obj);
    if(gensym("") == iemgui->x_lab) {
        /* put empty labels where they don't create scrollbars */
        dx = 0;
        dy = 7;
    }
    pdgui_vmess(0, "crs ii",
        glist_getcanvas(iemgui->x_glist), "coords", tag,
        x0  + dx*zoom, y0 + dy*zoom);
}

static void iemgui_dolabel(void *x, t_iemgui *iemgui, t_symbol *s, int senditup)
{
    t_symbol *empty = gensym("");
    t_symbol *old = iemgui->x_lab;
    s = s?canvas_realizedollar(iemgui->x_glist, s):0;
    if (!(s && s->s_name && s->s_name[0] && strcmp(s->s_name, "empty")))
        s = empty;
    iemgui->x_lab = s;

    if(senditup < 0) {
        senditup = (glist_isvisible(iemgui->x_glist) && iemgui->x_lab != old);
    }

    if(senditup)
    {
        const char*label = s->s_name;
        int have_label = (s != empty);
        char tag[128];
        sprintf(tag, "%p_LABEL", x);
        pdgui_vmess("pdtk_text_set", "cs s",
            glist_getcanvas(iemgui->x_glist), tag,
            have_label?s->s_name:"");
        iemgui_dolabelpos(x, iemgui);
    }
}

static void iemgui_label(void *x, t_iemgui *iemgui, t_symbol *s)
{
    iemgui->x_lab_unexpanded = s;
    iemgui_dolabel(x, iemgui, s, -1);
}


static void iemgui_label_pos(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    iemgui->x_ldx = (int)atom_getfloatarg(0, ac, av);
    iemgui->x_ldy = (int)atom_getfloatarg(1, ac, av);
    if(glist_isvisible(iemgui->x_glist))
        iemgui_dolabelpos(x, iemgui);
}

static void iemgui_label_font(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    int zoom = glist_getzoom(iemgui->x_glist);
    int f = (int)atom_getfloatarg(0, ac, av);

    if(f == 1) strcpy(iemgui->x_font, "helvetica");
    else if(f == 2) strcpy(iemgui->x_font, "times");
    else
    {
        f = 0;
        strcpy(iemgui->x_font, sys_font);
    }
    iemgui->x_fsf.x_font_style = f;
    f = (int)atom_getfloatarg(1, ac, av);
    if(f < 4)
        f = 4;
    iemgui->x_fontsize = f;
    if(glist_isvisible(iemgui->x_glist))
    {
        char tag[128];
        t_atom fontatoms[3];
        sprintf(tag, "%p_LABEL", x);
        SETSYMBOL(fontatoms+0, gensym(iemgui->x_font));
        SETFLOAT (fontatoms+1, -iemgui->x_fontsize*zoom);
        SETSYMBOL(fontatoms+2, gensym(sys_fontweight));
        pdgui_vmess(0, "crs rA",
            glist_getcanvas(iemgui->x_glist), "itemconfigure", tag,
            "-font", 3, fontatoms);
    }
}

static void iemgui_do_drawmove(void *x, t_iemgui*iemgui)
{
    if(glist_isvisible(iemgui->x_glist))
    {
        int xpos = text_xpix(&iemgui->x_obj, iemgui->x_glist);
        int ypos = text_ypix(&iemgui->x_obj, iemgui->x_glist);
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_MOVE);
        iemgui->x_private->p_prevX = xpos;
        iemgui->x_private->p_prevY = ypos;
        canvas_fixlinesfor(iemgui->x_glist, (t_text*)x);
    }
}

static void iemgui_size(void *x, t_iemgui *iemgui)
{
    if(glist_isvisible(iemgui->x_glist))
    {
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_CONFIG);
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_IO);
        canvas_fixlinesfor(iemgui->x_glist, (t_text*)x);
    }
}

static void iemgui_delta(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    int zoom = glist_getzoom(iemgui->x_glist);
    iemgui->x_obj.te_xpix += (int)atom_getfloatarg(0, ac, av);
    iemgui->x_obj.te_ypix += (int)atom_getfloatarg(1, ac, av);
    iemgui_do_drawmove(x, iemgui);
}

static void iemgui_pos(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    int zoom = glist_getzoom(iemgui->x_glist);
    iemgui->x_obj.te_xpix = (int)atom_getfloatarg(0, ac, av);
    iemgui->x_obj.te_ypix = (int)atom_getfloatarg(1, ac, av);
    iemgui_do_drawmove(x, iemgui);
}

static void iemgui_color(void *x, t_iemgui *iemgui, t_symbol *s, int ac, t_atom *av)
{
    if (ac >= 1)
        iemgui->x_bcol = _iemgui_compatible_colorarg(0, ac, av, BACKGROUND);
    if (ac == 2 && pd_compatibilitylevel < 47)
            /* old versions of Pd updated foreground and label color
            if only two args; now we do it more coherently. */
        iemgui->x_lcol = _iemgui_compatible_colorarg(1, ac, av, LABEL);
    else if (ac >= 2)
        iemgui->x_fcol = _iemgui_compatible_colorarg(1, ac, av, FOREGROUND);
    if (ac >= 3)
        iemgui->x_lcol = _iemgui_compatible_colorarg(2, ac, av, LABEL);
    if(glist_isvisible(iemgui->x_glist))
        (*iemgui->x_draw)(x, iemgui->x_glist, IEM_GUI_DRAW_MODE_CONFIG);
}

static void iemgui_displace(t_gobj *z, t_glist *glist, int dx, int dy)
{
    t_iemgui *x = (t_iemgui *)z;

    x->x_obj.te_xpix += dx;
    x->x_obj.te_ypix += dy;
    iemgui_do_drawmove(x, x);
}

static void iemgui_select(t_gobj *z, t_glist *glist, int selected)
{
    t_iemgui *x = (t_iemgui *)z;

    x->x_fsf.x_selected = selected;
    if(glist_isvisible(x->x_glist))
        (*x->x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_SELECT);
}

static void iemgui_delete(t_gobj *z, t_glist *glist)
{
    canvas_deletelinesfor(glist, (t_text *)z);
}

static void iemgui_vis(t_gobj *z, t_glist *glist, int vis)
{
    t_iemgui *x = (t_iemgui *)z;

    (*x->x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_ERASE);
    if (vis)
        (*x->x_draw)((void *)z, glist, IEM_GUI_DRAW_MODE_NEW);
    else
    {
        sys_unqueuegui(z);
    }
    x->x_private->p_prevX = text_xpix(&x->x_obj, glist);
    x->x_private->p_prevY = text_ypix(&x->x_obj, glist);
}

/* store saveable symbols (with spaces and dollars escaped) into srl[3] */
static void iemgui_save(t_iemgui *iemgui, t_symbol **srl, t_symbol**bflcol)
{
    int i;
    srl[0] = iemgui->x_snd;
    srl[1] = iemgui->x_rcv;
    srl[2] = iemgui->x_lab;
    iemgui_all_sym2dollararg(iemgui, srl);
    for(i=0; i<3; i++)
    {
        if(!srl[i] || !srl[i]->s_name || !srl[i]->s_name[0])
            srl[i]=gensym("empty");
    }
    iemgui_all_col2save(iemgui, bflcol);
}

    /* inform GUIs that glist's zoom is about to change.  The glist will
    take care of x,y locations but we have to adjust width and height */
static void iemgui_zoom(t_iemgui *iemgui, t_floatarg zoom)
{
    int oldzoom = iemgui->x_glist->gl_zoom;
    if (oldzoom < 1)
        oldzoom = 1;
    iemgui->x_w = (int)(iemgui->x_w)/oldzoom*(int)zoom;
    iemgui->x_h = (int)(iemgui->x_h)/oldzoom*(int)zoom;
}

    /* when creating a new GUI from menu onto a zoomed canvas, pretend to
    change the canvas's zoom so we'll get properly sized */
static void iemgui_newzoom(t_iemgui *iemgui)
{
    if (iemgui->x_glist->gl_zoom != 1)
    {
        int newzoom = iemgui->x_glist->gl_zoom;
        iemgui->x_glist->gl_zoom = 1;
        iemgui_zoom(iemgui, (t_floatarg)newzoom);
        iemgui->x_glist->gl_zoom = newzoom;
    }
}

/* escape characters for tcl/tk */
static char* iemgui_strnescape(char *dst, size_t dstlen, const char *src, size_t srclen)
{
    unsigned ptin = 0, ptout = 0;
    if(!dst || !src)return 0;
    while(1)
    {
        int c = src[ptin];
        if (c == '\\' || c == '{' || c == '}' || c == '[' || c == ']') {
            dst[ptout++] = '\\';
            if (dstlen && ptout >= dstlen){
                dst[ptout-1] = 0;
                break;
            }
        }
        dst[ptout] = c;
        ptin++;
        ptout++;
        if (c==0) break;
        if (srclen && ptin  >= srclen) break;
        if (dstlen && ptout >= dstlen) break;
    }

    if(!dstlen || ptout < dstlen)
        dst[ptout]=0;
    else
        dst[dstlen-1]=0;

    return dst;
}

static void iemgui_properties(t_iemgui *iemgui, t_symbol **srl)
{
    char label[MAXPDSTRING];
    int i;
    srl[0] = iemgui->x_snd;
    srl[1] = iemgui->x_rcv;
    srl[2] = iemgui->x_lab;

    iemgui_all_sym2dollararg(iemgui, srl);

    for(i=0; i<3; i++) {
        if(srl[i])
            srl[i] = gensym(iemgui_strnescape(label, sizeof(label), srl[i]->s_name, strlen(srl[i]->s_name)));
    }
}

#if 0
static void iemgui_new_dialog(void*x, t_iemgui*iemgui,
                       const char*objname,
                       t_float width,  t_float width_min,
                       t_float height, t_float height_min,
                       t_float range_min, t_float range_max,
                       int schedule,
                       int mode, /* lin0_log1 */
                       const char* label_mode0,
                       const char* label_mode1,
                       int canloadbang, int steady, int number)
{
    char objname_[MAXPDSTRING];
    t_symbol *srl[3];
    iemgui_properties(iemgui, srl);
    sprintf(objname_, "|%s|", objname);

    pdgui_stub_vnew(&iemgui->x_obj.ob_pd, "pdtk_iemgui_dialog", x,
        "r s ffs ffs sfsfs i iss ii si sss ii ii kkk",
        objname_,
        "",
        width, width_min, "",
        height, height_min, "",
        "", range_min, "", range_max, "",
        schedule,
        mode, label_mode0, label_mode1,
        canloadbang?iemgui->x_isa.x_loadinit:-1, steady,
        "", number,
        srl[0]?srl[0]->s_name:"", srl[1]?srl[1]->s_name:"", srl[2]?srl[2]->s_name:"",
        iemgui->x_ldx, iemgui->x_ldy,
        iemgui->x_fsf.x_font_style, iemgui->x_fontsize,
        iemgui->x_bcol, iemgui->x_fcol, iemgui->x_lcol);
}
#endif

static int iemgui_dialog(t_iemgui *iemgui, t_symbol **srl, int argc, t_atom *argv)
{
    char str[144];
    int init = (int)atom_getfloatarg(5, argc, argv);
    int ldx = (int)atom_getfloatarg(10, argc, argv);
    int ldy = (int)atom_getfloatarg(11, argc, argv);
    int f = (int)atom_getfloatarg(12, argc, argv);
    int fs = (int)atom_getfloatarg(13, argc, argv);
    int bcol = (int)iemgui_getcolorarg(14, argc, argv, BACKGROUND);
    int fcol = (int)iemgui_getcolorarg(15, argc, argv, FOREGROUND);
    int lcol = (int)iemgui_getcolorarg(16, argc, argv, LABEL);
    int rcv_changed=0, oldsndrcvable=0;
    int i;

    if(iemgui->x_fsf.x_rcv_able)
        oldsndrcvable |= IEM_GUI_OLD_RCV_FLAG;
    if(iemgui->x_fsf.x_snd_able)
        oldsndrcvable |= IEM_GUI_OLD_SND_FLAG;
    if(IS_A_SYMBOL(argv,7))
        srl[0] = atom_getsymbolarg(7, argc, argv);
    else if(IS_A_FLOAT(argv,7))
    {
        srl[0] = gensym("empty");
    }
    if(IS_A_SYMBOL(argv,8))
        srl[1] = atom_getsymbolarg(8, argc, argv);
    else if(IS_A_FLOAT(argv,8))
    {
        srl[1] = gensym("empty");
    }
    if(IS_A_SYMBOL(argv,9))
        srl[2] = atom_getsymbolarg(9, argc, argv);
    else if(IS_A_FLOAT(argv,9))
    {
        sprintf(str, "%g", atom_getfloatarg(9, argc, argv));
        srl[2] = gensym(str);
    }
    if(init != 0) init = 1;
    iemgui->x_isa.x_loadinit = init;
    for(i=0; i<3; i++)
        if(!srl_is_valid(srl[i]) || (!strcmp(srl[i]->s_name, "empty"))) srl[i] = &s_;

        /* expand dollargs
         * after this, srl holds the $-expanded versions of the labels
         * and iemgui->x_(snd|rcv|lab)_unexpanded hold the unexpanded versions
         */
    iemgui_all_dollararg2sym(iemgui, srl);

        /* check if the receiver changed */
    if(0
       || (!srl_is_valid(iemgui->x_rcv) && srl_is_valid(srl[1])) /* there was none, but now there is */
       || ( srl_is_valid(iemgui->x_rcv) && !(srl_is_valid(srl[1]))) /* there was one, but now there is */
       || ( srl_is_valid(iemgui->x_rcv) && srl_is_valid(srl[1]) && iemgui->x_rcv != srl[1])) /* both are valid, but changed */
        rcv_changed = 1;

        /* if the receiver changed (and was previously set), unbind it */
    if(rcv_changed && srl_is_valid(iemgui->x_rcv))
        pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);

    iemgui->x_snd = srl[0];
    iemgui->x_fsf.x_snd_able = srl_is_valid(srl[0]);
    iemgui->x_rcv = srl[1];
    iemgui->x_fsf.x_rcv_able = srl_is_valid(srl[1]);
    iemgui->x_lab = srl[2];
    iemgui->x_lcol = lcol & 0xffffff;
    iemgui->x_fcol = fcol & 0xffffff;
    iemgui->x_bcol = bcol & 0xffffff;
    iemgui->x_ldx = ldx;
    iemgui->x_ldy = ldy;
    if(f == 1) strcpy(iemgui->x_font, "helvetica");
    else if(f == 2) strcpy(iemgui->x_font, "times");
    else
    {
        f = 0;
        strcpy(iemgui->x_font, sys_font);
    }
    iemgui->x_fsf.x_font_style = f;
    if(fs < 4)
        fs = 4;
    iemgui->x_fontsize = fs;

        /* if the receiver changed (and is now set), bind it */
    if(rcv_changed && srl_is_valid(iemgui->x_rcv))
        pd_bind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);

    iemgui_verify_snd_ne_rcv(iemgui);
    canvas_dirty(iemgui->x_glist, 1);
    return(oldsndrcvable);
}

static void iemgui_setdialogatoms(t_iemgui *iemgui, int argc, t_atom*argv)
{
#define SETCOLOR(a, col) do {char color[MAXPDSTRING]; s_pd_snprintf(color, MAXPDSTRING-1, "#%06x", 0xffffff & col); color[MAXPDSTRING-1] = 0; SETSYMBOL(a, gensym(color));} while(0)
    t_float zoom = iemgui->x_glist->gl_zoom;
    t_symbol *srl[3];
    int for_undo = 1;
    int i;
    for(i=0; i<argc; i++)
        SETFLOAT(argv+i, -1); /* initialize */

    if(for_undo) {
        static t_symbol*s_empty = 0;
        if(!s_empty)s_empty = gensym("empty");
        srl[0] = iemgui->x_snd_unexpanded;
        srl[1] = iemgui->x_rcv_unexpanded;
        srl[2] = iemgui->x_lab_unexpanded;
        /* just in case one of the labels is NULL, set it to something valid */
        for(i=0; i<3; i++)
            if (!srl[i])
                srl[i]=s_empty;
    } else {
        iemgui_properties(iemgui, srl);
    }

    if(argc> 0) SETFLOAT (argv+ 0, iemgui->x_w/zoom);
    if(argc> 1) SETFLOAT (argv+ 1, iemgui->x_h/zoom);
    if(argc> 5) SETFLOAT (argv+ 5, iemgui->x_isa.x_loadinit);
    if(argc> 6) SETFLOAT (argv+ 6, 1); /* num */
    if(argc> 7) SETSYMBOL(argv+ 7, srl[0]);
    if(argc> 8) SETSYMBOL(argv+ 8, srl[1]);
    if(argc> 9) SETSYMBOL(argv+ 9, srl[2]);
    if(argc>10) SETFLOAT (argv+10, iemgui->x_ldx);
    if(argc>11) SETFLOAT (argv+11, iemgui->x_ldy);
    if(argc>12) SETFLOAT (argv+12, iemgui->x_fsf.x_font_style);
    if(argc>13) SETFLOAT (argv+13, iemgui->x_fontsize);
    if(argc>14) SETCOLOR (argv+14, iemgui->x_bcol);
    if(argc>15) SETCOLOR (argv+15, iemgui->x_fcol);
    if(argc>16) SETCOLOR (argv+16, iemgui->x_lcol);
}


/* pre-0.46 the flags were 1 for 'loadinit' and 1<<20 for 'scale'.
Starting in 0.46, take either 1<<20 or 1<<1 for 'scale' and save to both
bits (so that old versions can read files we write).  In the future (2015?)
we can stop writing the annoying  1<<20 bit. */
#define LOADINIT 1
#define SCALE 2
#define SCALEBIS (1<<20)

static void iem_inttosymargs(t_iem_init_symargs *symargp, int n)
{
    memset(symargp, 0, sizeof(*symargp));
    symargp->x_loadinit = ((n & LOADINIT) != 0);
    symargp->x_scale = ((n & SCALE) || (n & SCALEBIS)) ;
    symargp->x_flashed = 0;
    symargp->x_locked = 0;
}

static int iem_symargstoint(t_iem_init_symargs *symargp)
{
    return ((symargp->x_loadinit ? LOADINIT : 0) |
        (symargp->x_scale ? (SCALE | SCALEBIS) : 0));
}

static void iem_inttofstyle(t_iem_fstyle_flags *fstylep, int n)
{
    memset(fstylep, 0, sizeof(*fstylep));
    fstylep->x_font_style = (n >> 0);
    fstylep->x_shiftdown = 0;
    fstylep->x_selected = 0;
    fstylep->x_finemoved = 0;
    fstylep->x_put_in2out = 0;
    fstylep->x_change = 0;
    fstylep->x_thick = 0;
    fstylep->x_lin0_log1 = 0;
    fstylep->x_steady = 0;
}

static int iem_fstyletoint(t_iem_fstyle_flags *fstylep)
{
    return ((fstylep->x_font_style << 0) & 63);
}

static void iemgui_draw_new(t_iemgui*x, t_glist*glist) {;}
static void iemgui_draw_config(t_iemgui*x, t_glist*glist) {;}
static void iemgui_draw_update(t_iemgui*x, t_glist*glist) {;}
static void iemgui_draw_select(t_iemgui*x, t_glist*glist) {;}
static void iemgui_draw_iolets(t_iemgui*x, t_glist*glist, int old_snd_rcv_flags)
{
    const int zoom = x->x_glist->gl_zoom;
    int xpos = text_xpix(&x->x_obj, glist);
    int ypos = text_ypix(&x->x_obj, glist);
    int iow = IOWIDTH * zoom, ioh = IEM_GUI_IOHEIGHT * zoom;
    t_canvas *canvas = glist_getcanvas(glist);
    char tag_object[128], tag_label[128], tag[128];
    char *tags[] = {tag_object, tag};

    (void)old_snd_rcv_flags;

    sprintf(tag_object, "%p_", x);
    sprintf(tag_label, "%p_LABEL", x);

    /* re-create outlet */
    sprintf(tag, "%p_OUT%d", x, 0);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if(!x->x_fsf.x_snd_able) {
        pdgui_vmess(0, "crr iiii rk rk rS",
            canvas, "create", "rectangle",
            xpos, ypos + x->x_h + zoom - ioh, xpos + iow, ypos + x->x_h,
            "-fill", THISGUI->i_foregroundcolor,
            "-outline", THISGUI->i_foregroundcolor,
            "-tags", 2, tags);
        /* keep label above outlet */
        pdgui_vmess(0, "crss", canvas, "lower", tag, tag_label);
    }

    /* re-create inlet */
    sprintf(tag, "%p_IN%d", x, 0);
    pdgui_vmess(0, "crs", canvas, "delete", tag);
    if(!x->x_fsf.x_rcv_able) {
        pdgui_vmess(0, "crr iiii rk rk rS",
            canvas, "create", "rectangle",
            xpos, ypos, xpos + iow, ypos - zoom + ioh,
            "-fill", THISGUI->i_foregroundcolor,
            "-outline", THISGUI->i_foregroundcolor,
            "-tags", 2, tags);
        /* keep label above inlet */
        pdgui_vmess(0, "crss", canvas, "lower", tag, tag_label);
    }
}

static void iemgui_draw_erase(t_iemgui* x, t_glist* glist)
{
    t_canvas *canvas = glist_getcanvas(glist);
    char tag_object[128];
    sprintf(tag_object, "%p_", x);

    pdgui_vmess(0, "crs", canvas, "delete", tag_object);
}

static void iemgui_draw_move(t_iemgui *x, t_glist *glist)
{
    t_canvas *canvas = glist_getcanvas(glist);
    int dx = text_xpix(&x->x_obj, glist) - x->x_private->p_prevX;
    int dy = text_ypix(&x->x_obj, glist) - x->x_private->p_prevY;

    char tag_object[128];
    sprintf(tag_object, "%p_", x);

    //pdgui_vmess(0, "rcs ii", "pdtk_canvas_move", canvas, tag_object, dx, dy);
    pdgui_vmess(0, "crs ii", canvas, "move", tag_object, dx, dy);
}

static void iemgui_draw(t_iemgui *x, t_glist *glist, int mode)
{
#define DRAW_FUN(fun, x, glist) {                                       \
        t_iemdrawfunptr do_draw = x->x_private->p_widget.draw_##fun;    \
        if (!do_draw) do_draw = (t_iemdrawfunptr)iemgui_draw_##fun;     \
        do_draw(x, glist);                                              \
    }

    switch(mode) {
    case (IEM_GUI_DRAW_MODE_UPDATE):
    {
        t_iemdrawfunptr draw_update = x->x_private->p_widget.draw_update;
        if(!draw_update)
            draw_update = (t_iemdrawfunptr)iemgui_draw_update;
        sys_queuegui(x, x->x_glist, (t_guicallbackfn)draw_update);
    }
        break;
    case (IEM_GUI_DRAW_MODE_MOVE):
        DRAW_FUN(move, x, glist);
        break;
    case (IEM_GUI_DRAW_MODE_NEW):
        DRAW_FUN(new, x, glist);
        break;
    case (IEM_GUI_DRAW_MODE_SELECT):
        DRAW_FUN(select, x, glist);
        break;
    case (IEM_GUI_DRAW_MODE_ERASE):
        DRAW_FUN(erase, x, glist);
        break;
    case (IEM_GUI_DRAW_MODE_CONFIG):
        DRAW_FUN(config, x, glist);
        break;
    default:
        if(x->x_private->p_widget.draw_iolets)
            x->x_private->p_widget.draw_iolets(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
        else
            iemgui_draw_iolets(x, glist, mode - IEM_GUI_DRAW_MODE_IO);
    }
}

static void iemgui_setdrawfunctions(t_iemgui *iemgui, t_iemgui_drawfunctions *w)
{
#define SET_DRAW(x, fun) \
    x->x_private->p_widget.draw_##fun = w->draw_##fun

    SET_DRAW(iemgui, new);
    SET_DRAW(iemgui, config);
    SET_DRAW(iemgui, iolets);
    SET_DRAW(iemgui, update);
    SET_DRAW(iemgui, select);
    SET_DRAW(iemgui, erase);
    SET_DRAW(iemgui, move);
}

#define IEMGUI_SETDRAWFUNCTIONS(x, prefix)                     \
    {                                                            \
        t_iemgui_drawfunctions w;                              \
        w.draw_new    = (t_iemdrawfunptr)prefix##_draw_new;      \
        w.draw_config = (t_iemdrawfunptr)prefix##_draw_config;   \
        w.draw_iolets = (t_iemfunptr)prefix##_draw_io;       \
        w.draw_update = (t_iemdrawfunptr)prefix##_draw_update;   \
        w.draw_select = (t_iemdrawfunptr)prefix##_draw_select;   \
        w.draw_erase = 0;                                        \
        w.draw_move  = 0;                                        \
        iemgui_setdrawfunctions(&x->x_gui, &w);                        \
    }

static void iemgui_free(t_iemgui *iemgui) {
    if(iemgui->x_fsf.x_rcv_able)
        pd_unbind(&iemgui->x_obj.ob_pd, iemgui->x_rcv);
    pdgui_stub_deleteforkey(iemgui);
    sys_unqueuegui(iemgui);

    freebytes(iemgui->x_private, sizeof(*iemgui->x_private));
}

static t_iemgui *iemgui_new(t_class*cls)
{
    t_iemgui *x = (t_iemgui *)pd_new(cls);
    t_glist *cnv = canvas_getcurrent();
    int fs = cnv->gl_font;
    x->x_glist = cnv;
    x->x_private = (t_iemgui_private*)getbytes(sizeof(*x->x_private));
    x->x_draw = (t_iemfunptr)iemgui_draw;

    x->x_fontsize = (fs<4)?4:fs;
/*
    int fs = x->x_gui.x_fontsize;
    x->x_gui.x_fontsize = (fs < 4)?4:fs;
*/
    iem_inttosymargs(&x->x_isa, 0);
    iem_inttofstyle(&x->x_fsf, 0);

    x->x_bcol = IEM_GUI_COLOR_BACKGROUND;
    x->x_fcol = IEM_GUI_COLOR_FOREGROUND;
    x->x_lcol = IEM_GUI_COLOR_LABEL;

    return x;
}

#endif // __iemgui_static_h_

