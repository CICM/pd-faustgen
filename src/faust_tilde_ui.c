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
    t_symbol*   p_name;
    t_symbol*   p_longname;
    int         p_type;
    FAUSTFLOAT* p_zone;
    FAUSTFLOAT  p_min;
    FAUSTFLOAT  p_max;
    FAUSTFLOAT  p_step;
    FAUSTFLOAT  p_default;
    FAUSTFLOAT  p_saved;
}t_faust_ui;

typedef struct _faust_ui_manager
{
    UIGlue      f_glue;
    t_object*   f_owner;
    t_clock*    f_clock;
    t_faust_ui* f_active_uis;
    size_t      f_nactive_uis;
    t_faust_ui* f_temp_ui;
    t_faust_ui* f_passive_uis;
    size_t      f_npassive_uis;
    t_symbol**  f_names;
    size_t      f_nnames;
    MetaGlue    f_meta_glue;
}t_faust_ui_manager;


static void faust_ui_manager_clear_active_uis(t_faust_ui_manager *x)
{
    if(x->f_active_uis && x->f_nactive_uis)
    {
        freebytes(x->f_active_uis, x->f_nactive_uis * sizeof(t_faust_ui));
    }
    x->f_active_uis    = NULL;
    x->f_nactive_uis   = 0;
}

static void faust_ui_manager_clear_passive_uis(t_faust_ui_manager *x)
{
    if(x->f_passive_uis && x->f_npassive_uis)
    {
        freebytes(x->f_passive_uis, x->f_npassive_uis * sizeof(t_faust_ui));
    }
    x->f_passive_uis   = NULL;
    x->f_npassive_uis  = 0;
}

static void faust_ui_manager_clear_names(t_faust_ui_manager *x)
{
    if(x->f_names && x->f_nnames)
    {
        freebytes(x->f_names, x->f_nnames * sizeof(t_symbol *));
    }
    x->f_names  = NULL;
    x->f_nnames = 0;
}

static t_faust_ui* faust_ui_manager_get_active_uis(t_faust_ui_manager *x, t_symbol* name)
{
    size_t i;
    for(i = 0; i < x->f_nactive_uis; ++i)
    {
        if(x->f_active_uis[i].p_name == name ||
           x->f_active_uis[i].p_longname == name)
        {
            return x->f_active_uis+i;
        }
    }
    return NULL;
}

static t_faust_ui* faust_ui_manager_get_passive_uis(t_faust_ui_manager *x, t_symbol* name)
{
    size_t i;
    for(i = 0; i < x->f_npassive_uis; ++i)
    {
        if(x->f_passive_uis[i].p_name == name ||
           x->f_passive_uis[i].p_longname == name)
        {
            return x->f_passive_uis+i;
        }
    }
    return NULL;
}

static t_symbol* faust_ui_manager_get_long_name(t_faust_ui_manager *x, const char* label)
{
    size_t i;
    char name[MAXFAUSTSTRING];
    memset(name, 0, MAXFAUSTSTRING);
    for(i = 0; i < x->f_nnames; ++i)
    {
        strncat(name, x->f_names[i]->s_name, MAXFAUSTSTRING);
        strncat(name, "/", 1);
    }
    strncat(name, label, MAXFAUSTSTRING);
    return gensym(name);
}

static void faust_ui_manager_add_params(t_faust_ui_manager *x, const char* label, int const type, FAUSTFLOAT* zone,
                                        FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    t_faust_ui *newmemory = NULL;
    t_faust_ui *oldmemory = (type == FAUST_UI_TYPE_BARGRAPH) ? x->f_passive_uis : x->f_active_uis;
    size_t size = (type == FAUST_UI_TYPE_BARGRAPH) ? x->f_npassive_uis : x->f_nactive_uis;
    if(oldmemory)
    {
        newmemory = (t_faust_ui *)resizebytes(oldmemory, size * sizeof(t_faust_ui), (size + 1) * sizeof(t_faust_ui));
        if(!newmemory)
        {
            pd_error(x->f_owner, "faustgen~: memory allocation failed - ui glue");
            return;
        }
    }
    else
    {
        newmemory = (t_faust_ui *)getbytes(sizeof(t_faust_ui));
        if(!newmemory)
        {
            pd_error(x->f_owner, "faustgen~: memory allocation failed - ui glue");
            return;
        }
    }

    newmemory[size].p_name      = gensym(label);
    newmemory[size].p_longname  = faust_ui_manager_get_long_name(x, label);
    newmemory[size].p_type      = type;
    newmemory[size].p_zone      = zone;
    newmemory[size].p_min       = min;
    newmemory[size].p_max       = max;
    newmemory[size].p_step      = step;
    newmemory[size].p_default   = init;
    newmemory[size].p_saved     = init;
    *(newmemory[size].p_zone)   = init;
    if(type == FAUST_UI_TYPE_BARGRAPH)
    {
        x->f_passive_uis  = (t_faust_ui *)newmemory;
        x->f_npassive_uis = size + 1;
    }
    else
    {
        x->f_active_uis  = (t_faust_ui *)newmemory;
        x->f_nactive_uis = size + 1;
    }
    
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
    faust_ui_manager_add_params(x, label, FAUST_UI_TYPE_BUTTON, zone, 0, 0, 0, 0);
}

static void faust_ui_manager_ui_add_toggle(t_faust_ui_manager* x, const char* label, FAUSTFLOAT* zone)
{
    faust_ui_manager_add_params(x, label, FAUST_UI_TYPE_TOGGLE, zone, 0, 0, 1, 1);
}

static void faust_ui_manager_ui_add_number(t_faust_ui_manager* x, const char* label, FAUSTFLOAT* zone,
                                            FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    faust_ui_manager_add_params(x, label, FAUST_UI_TYPE_NUMBER, zone, init, min, max, step);
}

// PASSIVE UIS
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_ui_manager_ui_add_bargraph(t_faust_ui_manager* x, const char* label,
                                                        FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
    faust_ui_manager_add_params(x, label, FAUST_UI_TYPE_BARGRAPH, zone, 0, min, max, 0);
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

static void faust_ui_manager_tick(t_faust_ui_manager* x);

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
        ui_manager->f_glue.addSoundFile           = (addSoundFileFun)faust_ui_manager_ui_add_sound_file;
        ui_manager->f_glue.declare                = (declareFun)faust_ui_manager_ui_declare;
        
        ui_manager->f_owner         = owner;
        ui_manager->f_clock         = clock_new(ui_manager, (t_method)faust_ui_manager_tick);
        ui_manager->f_active_uis    = NULL;
        ui_manager->f_nactive_uis   = 0;
        ui_manager->f_passive_uis   = NULL;
        ui_manager->f_npassive_uis  = 0;
        ui_manager->f_names         = NULL;
        ui_manager->f_nnames        = 0;
        
        ui_manager->f_meta_glue.metaInterface = ui_manager;
        ui_manager->f_meta_glue.declare       = (metaDeclareFun)faust_ui_manager_meta_declare;
    }
    return ui_manager;
}

void faust_ui_manager_free(t_faust_ui_manager *x)
{
    faust_ui_manager_clear(x);
    freebytes(x, sizeof(t_faust_ui_manager));
}

void faust_ui_manager_init(t_faust_ui_manager *x, void* dspinstance)
{
    faust_ui_manager_clear(x);
    buildUserInterfaceCDSPInstance((llvm_dsp *)dspinstance, (UIGlue *)&(x->f_glue));
    faust_ui_manager_clear_names(x);
    metadataCDSPInstance((llvm_dsp *)dspinstance, &x->f_meta_glue);
}

void faust_ui_manager_clear(t_faust_ui_manager *x)
{
    faust_ui_manager_clear_active_uis(x);
    faust_ui_manager_clear_passive_uis(x);
    faust_ui_manager_clear_names(x);
}

static void faust_ui_manager_tick(t_faust_ui_manager* x)
{
    if(x->f_temp_ui)
    {
        *(x->f_temp_ui->p_zone) = 0;
    }
    x->f_temp_ui = NULL;
}

char faust_ui_manager_has_passive_ui(t_faust_ui_manager *x)
{
    return x->f_npassive_uis > 0;
}

char faust_ui_manager_set(t_faust_ui_manager *x, t_symbol* name, t_float f)
{
    t_faust_ui* ui = faust_ui_manager_get_active_uis(x, name);
    if(ui)
    {
        if(ui->p_type == FAUST_UI_TYPE_BUTTON)
        {
            *(ui->p_zone) = 0;
            *(ui->p_zone) = 1;
            x->f_temp_ui = ui;
            clock_delay(x->f_clock, 1.5);
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
            else if(v > ui->p_max)
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

char faust_ui_manager_get(t_faust_ui_manager *x, t_symbol* name, t_float* f)
{
    t_faust_ui* ui = faust_ui_manager_get_passive_uis(x, name);
    if(ui)
    {
        *f = (t_float)(*(ui->p_zone));
        return 0;
    }
    return 1;
}

void faust_ui_manager_save_states(t_faust_ui_manager *x)
{
    size_t i;
    for(i = 0; i < x->f_nactive_uis; ++i)
    {
        x->f_active_uis[i].p_saved = *(x->f_active_uis[i].p_zone);
    }
}

void faust_ui_manager_restore_states(t_faust_ui_manager *x)
{
    size_t i;
    for(i = 0; i < x->f_nactive_uis; ++i)
    {
        *(x->f_active_uis[i].p_zone) = x->f_active_uis[i].p_saved;
    }
}

void faust_ui_manager_restore_default(t_faust_ui_manager *x)
{
    size_t i;
    for(i = 0; i < x->f_nactive_uis; ++i)
    {
        *(x->f_active_uis[i].p_zone) = x->f_active_uis[i].p_default;
    }
}

const char* faust_ui_manager_get_parameter_char(int type)
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

void faust_ui_manager_print(t_faust_ui_manager* x, char const log)
{
    size_t i;
    logpost(x->f_owner, 2+log, "             active parameters: %i", (int)x->f_nactive_uis);
    for(i = 0; i < x->f_nactive_uis; ++i)
    {
        logpost(x->f_owner, 2+log, "             %i: %s [path:%s - type:%s - init:%g - min:%g - max:%g - current:%g]", (int)i,
                x->f_active_uis[i].p_name->s_name,
                x->f_active_uis[i].p_longname->s_name,
                faust_ui_manager_get_parameter_char(x->f_active_uis[i].p_type),
                x->f_active_uis[i].p_default,
                x->f_active_uis[i].p_min,
                x->f_active_uis[i].p_max,
                *x->f_active_uis[i].p_zone);
    }
    logpost(x->f_owner, 2+log, "             passive parameters: %i", (int)x->f_npassive_uis);
    for(i = 0; i < x->f_npassive_uis; ++i)
    {
        logpost(x->f_owner, 2+log, "             %i: %s [path:%s - type:%s - init:%g - min:%g - max:%g - current:%g]", (int)i,
                x->f_passive_uis[i].p_name->s_name,
                x->f_passive_uis[i].p_longname->s_name,
                faust_ui_manager_get_parameter_char(x->f_passive_uis[i].p_type),
                x->f_passive_uis[i].p_default,
                x->f_passive_uis[i].p_min,
                x->f_passive_uis[i].p_max,
                *x->f_passive_uis[i].p_zone);
    }
}


