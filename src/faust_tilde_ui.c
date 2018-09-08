/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/


#include "faust_tilde_ui.h"
#include <faust/dsp/llvm-c-dsp.h>
#include <string.h>
#include <float.h>

#define MAXFAUSTSTRING 4096
#define FAUST_UI_TYPE_BUTTON     0
#define FAUST_UI_TYPE_TOGGLE     1
#define FAUST_UI_TYPE_NUMBER     2
#define FAUST_UI_TYPE_BARGRAPH   3

typedef struct _faust_ui
{
    t_symbol*           p_name;
    t_symbol*           p_longname;
    int                 p_type;
    FAUSTFLOAT*         p_zone;
    FAUSTFLOAT          p_min;
    FAUSTFLOAT          p_max;
    FAUSTFLOAT          p_step;
    FAUSTFLOAT          p_default;
    FAUSTFLOAT          p_saved;
    char                p_kept;
    FAUSTFLOAT          p_tempv;
    struct _faust_ui*   p_next;
}t_faust_ui;

typedef struct _faust_ui_manager
{
    UIGlue      f_glue;
    t_object*   f_owner;
    t_faust_ui* f_uis;
    t_symbol**  f_names;
    size_t      f_nnames;
    MetaGlue    f_meta_glue;
}t_faust_ui_manager;

static void faust_ui_manager_free_uis(t_faust_ui_manager *x)
{
    t_faust_ui *c = x->f_uis;
    while(c)
    {
        x->f_uis = c->p_next;
        freebytes(c, sizeof(*c));
        c = x->f_uis;
    }
}

static t_faust_ui* faust_ui_manager_get(t_faust_ui_manager const *x, t_symbol const *name)
{
    t_faust_ui *c = x->f_uis;
    while(c)
    {
        if(c->p_name == name || c->p_longname == name)
        {
            return c;
        }
        c = c->p_next;
    }
    return NULL;
}

static void faust_ui_manager_prepare_changes(t_faust_ui_manager *x)
{
    t_faust_ui *c = x->f_uis;
    while(c)
    {
        c->p_kept  = 0;
        c->p_tempv = *(c->p_zone);
        c = c->p_next;
    }
}

static void faust_ui_manager_finish_changes(t_faust_ui_manager *x)
{
    t_faust_ui *c = x->f_uis;
    if(c)
    {
        t_faust_ui *n = c->p_next;
        while(n)
        {
            if(!n->p_kept)
            {
                c->p_next = n->p_next;
                freebytes(n, sizeof(*n));
                n = c->p_next;
            }
            else
            {
                c = n;
                n = c->p_next;
            }
        }
        if(!x->f_uis->p_kept)
        {
            x->f_uis = x->f_uis->p_next;
            freebytes(x->f_uis, sizeof(*x->f_uis));
        }
    }
}

static void faust_ui_manager_free_names(t_faust_ui_manager *x)
{
    if(x->f_names && x->f_nnames)
    {
        freebytes(x->f_names, x->f_nnames * sizeof(t_symbol *));
    }
    x->f_names  = NULL;
    x->f_nnames = 0;
}

static t_symbol* faust_ui_manager_get_long_name(t_faust_ui_manager *x, const char* label)
{
    size_t i;
    char name[MAXFAUSTSTRING];
    memset(name, 0, MAXFAUSTSTRING);
    for(i = 0; i < x->f_nnames; ++i)
    {
        strncat(name, x->f_names[i]->s_name, MAXFAUSTSTRING - strnlen(name, MAXFAUSTSTRING) - 1);
        strncat(name, "/", MAXFAUSTSTRING - strnlen(name, MAXFAUSTSTRING) - 1);
    }
    strncat(name, label, MAXFAUSTSTRING - strnlen(name, MAXFAUSTSTRING) - 1);
    return gensym(name);
}

static void faust_ui_manager_add_param(t_faust_ui_manager *x, const char* label, int const type, FAUSTFLOAT* zone,
                                        FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    FAUSTFLOAT saved, current;
    t_symbol* name  = gensym(label);
    t_symbol* lname = faust_ui_manager_get_long_name(x, label);
    t_faust_ui *c   = faust_ui_manager_get(x, name);
    if(c)
    {
        saved   = c->p_saved;
        current = c->p_tempv;
    }
    else
    {
        c = (t_faust_ui *)getbytes(sizeof(*c));
        if(!c)
        {
            pd_error(x->f_owner, "faustgen~: memory allocation failed - ui glue");
            return;
        }
        c->p_name   = name;
        c->p_next   = x->f_uis;
        x->f_uis    = c;
        saved       = init;
        current     = init;
    }
    c->p_longname  = lname;
    c->p_type      = type;
    c->p_zone      = zone;
    c->p_min       = min;
    c->p_max       = max;
    c->p_step      = step;
    c->p_default   = init;
    c->p_saved     = saved;
    c->p_kept      = 1;
    *(c->p_zone)   = current;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                      PRIVATE INTERFACE                                       //
//////////////////////////////////////////////////////////////////////////////////////////////////

// NAME PATH
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_ui_manager_ui_open_box(t_faust_ui_manager* x, const char* label)
{
    if(x->f_nnames)
    {
        t_symbol** temp = (t_symbol**)resizebytes(x->f_names, x->f_nnames * sizeof(t_symbol *), (x->f_nnames + 1) * sizeof(t_symbol *));
        if(temp)
        {
            x->f_names  = temp;
            x->f_names[x->f_nnames] = gensym(label);
            x->f_nnames = x->f_nnames + 1;
            return;
        }
        else
        {
            pd_error(x->f_owner, "faustgen~: memory allocation failed - ui box");
            return;
        }
    }
    else
    {
        x->f_names = getbytes(sizeof(t_symbol *));
        if(x->f_names)
        {
            x->f_names[0] = gensym(label);
            x->f_nnames = 1;
            return;
        }
        else
        {
            pd_error(x->f_owner, "faustgen~: memory allocation failed - ui box");
            return;
        }
    }
}

static void faust_ui_manager_ui_close_box(t_faust_ui_manager* x)
{
    if(x->f_nnames > 1)
    {
        t_symbol** temp = (t_symbol**)resizebytes(x->f_names, x->f_nnames * sizeof(t_symbol *), (x->f_nnames - 1) * sizeof(t_symbol *));
        if(temp)
        {
            x->f_names  = temp;
            x->f_nnames = x->f_nnames - 1;
            return;
        }
        else
        {
            pd_error(x->f_owner, "faustgen~: memory de-allocation failed - ui box");
            return;
        }
    }
    else if(x->f_nnames)
    {
        freebytes(x->f_names, sizeof(t_symbol *));
        x->f_names  = NULL;
        x->f_nnames = 0;
    }
}


// ACTIVE UIS
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_ui_manager_ui_add_button(t_faust_ui_manager* x, const char* label, FAUSTFLOAT* zone)
{
    faust_ui_manager_add_param(x, label, FAUST_UI_TYPE_BUTTON, zone, 0, 0, 0, 0);
}

static void faust_ui_manager_ui_add_toggle(t_faust_ui_manager* x, const char* label, FAUSTFLOAT* zone)
{
    faust_ui_manager_add_param(x, label, FAUST_UI_TYPE_TOGGLE, zone, 0, 0, 1, 1);
}

static void faust_ui_manager_ui_add_number(t_faust_ui_manager* x, const char* label, FAUSTFLOAT* zone,
                                            FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    faust_ui_manager_add_param(x, label, FAUST_UI_TYPE_NUMBER, zone, init, min, max, step);
}

// PASSIVE UIS
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_ui_manager_ui_add_bargraph(t_faust_ui_manager* x, const char* label,
                                                        FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
    faust_ui_manager_add_param(x, label, FAUST_UI_TYPE_BARGRAPH, zone, 0, min, max, 0);
}

static void faust_ui_manager_ui_add_sound_file(t_faust_ui_manager* x, const char* label, const char* filename, struct Soundfile** sf_zone)
{
    pd_error(x->f_owner, "faustgen~: add sound file not supported yet");
}

// DECLARE UIS
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_ui_manager_ui_declare(t_faust_ui_manager* x, FAUSTFLOAT* zone, const char* key, const char* value)
{
    //logpost(x->f_owner, 3, "%s: %s - %f", key, value, *zone);
}

// META UIS
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_ui_manager_meta_declare(t_faust_ui_manager* x, const char* key, const char* value)
{
    logpost(x->f_owner, 3, "             %s: %s", key, value);
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//                                      PUBLIC INTERFACE                                        //
//////////////////////////////////////////////////////////////////////////////////////////////////

t_faust_ui_manager* faust_ui_manager_new(t_object* owner)
{
    t_faust_ui_manager* ui_manager = (t_faust_ui_manager*)getbytes(sizeof(t_faust_ui_manager));
    if(ui_manager)
    {
        ui_manager->f_glue.uiInterface            = ui_manager;
        ui_manager->f_glue.openTabBox             = (openTabBoxFun)faust_ui_manager_ui_open_box;
        ui_manager->f_glue.openHorizontalBox      = (openHorizontalBoxFun)faust_ui_manager_ui_open_box;
        ui_manager->f_glue.openVerticalBox        = (openVerticalBoxFun)faust_ui_manager_ui_open_box;
        ui_manager->f_glue.closeBox               = (closeBoxFun)faust_ui_manager_ui_close_box;
        
        ui_manager->f_glue.addButton              = (addButtonFun)faust_ui_manager_ui_add_button;
        ui_manager->f_glue.addCheckButton         = (addCheckButtonFun)faust_ui_manager_ui_add_toggle;
        ui_manager->f_glue.addVerticalSlider      = (addVerticalSliderFun)faust_ui_manager_ui_add_number;
        ui_manager->f_glue.addHorizontalSlider    = (addHorizontalSliderFun)faust_ui_manager_ui_add_number;
        ui_manager->f_glue.addNumEntry            = (addNumEntryFun)faust_ui_manager_ui_add_number;
        
        ui_manager->f_glue.addHorizontalBargraph  = (addHorizontalBargraphFun)faust_ui_manager_ui_add_bargraph;
        ui_manager->f_glue.addVerticalBargraph    = (addVerticalBargraphFun)faust_ui_manager_ui_add_bargraph;
        ui_manager->f_glue.addSoundfile           = (addSoundfileFun)faust_ui_manager_ui_add_sound_file;
        ui_manager->f_glue.declare                = (declareFun)faust_ui_manager_ui_declare;
        
        ui_manager->f_owner     = owner;
        ui_manager->f_uis       = NULL;
        ui_manager->f_names     = NULL;
        ui_manager->f_nnames    = 0;
        
        ui_manager->f_meta_glue.metaInterface = ui_manager;
        ui_manager->f_meta_glue.declare       = (metaDeclareFun)faust_ui_manager_meta_declare;
    }
    return ui_manager;
}

void faust_ui_manager_free(t_faust_ui_manager *x)
{
    faust_ui_manager_clear(x);
    freebytes(x, sizeof(*x));
}

void faust_ui_manager_init(t_faust_ui_manager *x, void* dspinstance)
{
    faust_ui_manager_prepare_changes(x);
    buildUserInterfaceCDSPInstance((llvm_dsp *)dspinstance, (UIGlue *)&(x->f_glue));
    faust_ui_manager_finish_changes(x);
    faust_ui_manager_free_names(x);
    metadataCDSPInstance((llvm_dsp *)dspinstance, &x->f_meta_glue);
}

void faust_ui_manager_clear(t_faust_ui_manager *x)
{
    faust_ui_manager_free_uis(x);
    faust_ui_manager_free_names(x);
}

char faust_ui_manager_set_value(t_faust_ui_manager *x, t_symbol const *name, t_float const f)
{
    t_faust_ui* ui = faust_ui_manager_get(x, name);
    if(ui)
    {
        if(ui->p_type == FAUST_UI_TYPE_BUTTON)
        {
            if(f > FLT_EPSILON)
            {
                *(ui->p_zone) = 0;
                *(ui->p_zone) = 1;
            }
            else
            {
                *(ui->p_zone) = 0;
            }
            return 0;
        }
        else if(ui->p_type == FAUST_UI_TYPE_TOGGLE)
        {
            *(ui->p_zone) = (FAUSTFLOAT)(f > FLT_EPSILON);
            return 0;
        }
        else if(ui->p_type == FAUST_UI_TYPE_NUMBER)
        {
            const FAUSTFLOAT v = (FAUSTFLOAT)(f);
            if(v < ui->p_min)
            {
                *(ui->p_zone) = ui->p_min;
                return 0;
            }
            if(v > ui->p_max)
            {
                *(ui->p_zone) = ui->p_max;
                return 0;
            }
            *(ui->p_zone) = v;
            return 0;
        }
    }
    return 1;
}

char faust_ui_manager_get_value(t_faust_ui_manager const *x, t_symbol const *name, t_float* f)
{
    t_faust_ui* ui = faust_ui_manager_get(x, name);
    if(ui)
    {
        *f = (t_float)(*(ui->p_zone));
        return 0;
    }
    return 1;
}

void faust_ui_manager_save_states(t_faust_ui_manager *x)
{
    t_faust_ui *c = x->f_uis;
    while(c)
    {
        c->p_saved = *(c->p_zone);
        c = c->p_next;
    }
}

void faust_ui_manager_restore_states(t_faust_ui_manager *x)
{
    t_faust_ui *c = x->f_uis;
    while(c)
    {
        *(c->p_zone) = c->p_saved;
        c = c->p_next;
    }
}

void faust_ui_manager_restore_default(t_faust_ui_manager *x)
{
    t_faust_ui *c = x->f_uis;
    while(c)
    {
        *(c->p_zone) = c->p_default;
        c = c->p_next;
    }
}

static const char* faust_ui_manager_get_parameter_char(int const type)
{
    if(type == FAUST_UI_TYPE_BUTTON)
        return "button";
    else if(type == FAUST_UI_TYPE_TOGGLE)
        return "toggle";
    else if(type == FAUST_UI_TYPE_NUMBER)
        return "number";
    else
        return "bargraph";
}

void faust_ui_manager_print(t_faust_ui_manager const *x, char const log)
{
    t_faust_ui *c = x->f_uis;
    while(c)
    {
        logpost(x->f_owner, 2+log, "             parameter: %s [path:%s - type:%s - init:%g - min:%g - max:%g - current:%g]",
                c->p_name->s_name, c->p_longname->s_name,
                faust_ui_manager_get_parameter_char(c->p_type),
                c->p_default, c->p_min, c->p_max, *c->p_zone);
        c = c->p_next;
    }
}


