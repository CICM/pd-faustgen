/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <m_pd.h>

#include <g_canvas.h>
#include <string.h>

#include <faust/dsp/llvm-c-dsp.h>

#define MAXFAUSTSTRING 4096
#define MAXFAUSTOPTIONS 128

typedef struct _faust_param
{
    t_symbol*   p_label;
    int         p_type;
    FAUSTFLOAT* p_zone;
    FAUSTFLOAT  p_min;
    FAUSTFLOAT  p_max;
    FAUSTFLOAT  p_step;
    FAUSTFLOAT  p_saved;
}t_faust_param;

typedef struct _faust_tilde
{
    t_object            f_obj;
    llvm_dsp_factory*   f_dsp_factory;
    llvm_dsp*           f_dsp_instance;
    size_t              f_nsignals;
    t_sample**          f_signals;
    t_float             f_f;
    
    MetaGlue            f_meta_glue;
    UIGlue              f_ui_glue;
    size_t              f_nparams;
    t_faust_param*      f_params;
    
    t_canvas*           f_canvas;
    t_symbol*           f_dsp_name;
    
    size_t              f_ncompile_options;
    char*               f_compile_options[MAXFAUSTOPTIONS];
    
    size_t              f_ninlets;
    t_inlet**           f_inlets;
    size_t              f_noutlets;
    t_outlet**          f_outlets;
} t_faust_tilde;

static t_class *faust_tilde_class;

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                      PURE DATA IO DYNAMIC                                    //
//////////////////////////////////////////////////////////////////////////////////////////////////
static void faust_tilde_free_ioputs(t_faust_tilde *x)
{
    if(x->f_inlets)
    {
        freebytes(x->f_inlets, sizeof(t_inlet*) * x->f_ninlets);
        x->f_inlets  = NULL;
        x->f_ninlets = 0;
    }
    if(x->f_outlets)
    {
        freebytes(x->f_outlets, sizeof(t_outlet*) * x->f_noutlets);
        x->f_outlets  = NULL;
        x->f_noutlets = 0;
    }
    if(x->f_signals)
    {
        freebytes(x->f_signals, sizeof(t_sample*) * x->f_nsignals);
        x->f_signals  = NULL;
        x->f_nsignals = 0;
    }
}

static char faust_tilde_resize_inputs(t_faust_tilde *x, int const nins)
{
    t_inlet** ninlets;
    size_t i;
    size_t const cins = x->f_ninlets > 1 ? x->f_ninlets : 1;
    size_t const rnins = (size_t)nins > 1 ? (size_t)nins : 1;
    if(rnins == cins)
    {
        return 0;
    }
    for(i = rnins; i < cins; ++i)
    {
        inlet_free(x->f_inlets[i]);
        x->f_inlets[i] = NULL;
    }
    ninlets = (t_inlet **)resizebytes(x->f_inlets, sizeof(t_inlet*) * cins, sizeof(t_inlet*) * rnins);
    if(ninlets)
    {
        for(i = cins; i < rnins; ++i)
        {
            ninlets[i] = signalinlet_new((t_object *)x, 0);
        }
        x->f_inlets = ninlets;
        x->f_ninlets = rnins;
        return 0;
    }
    pd_error(x, "faust~: memory allocation failed - inputs");
    return 1;
}

static char faust_tilde_resize_outputs(t_faust_tilde *x, int const nins)
{
    t_outlet** noutlets;
    size_t i;
    size_t const couts = x->f_noutlets;
    size_t const rnouts = (size_t)nins > 0 ? (size_t)nins : 0;
    if(rnouts == couts)
    {
        return 0;
    }
    for(i = rnouts; i < couts; ++i)
    {
        outlet_free(x->f_outlets[i]);
        x->f_outlets[i] = NULL;
    }
    noutlets = (t_outlet **)resizebytes(x->f_outlets, sizeof(t_outlet*) * couts, sizeof(t_outlet*) * rnouts);
    if(noutlets)
    {
        for(i = couts; i < rnouts; ++i)
        {
            noutlets[i] = outlet_new((t_object *)x, &s_signal);
        }
        x->f_outlets = noutlets;
        x->f_noutlets = rnouts;
        return 0;
    }
    pd_error(x, "faust~: memory allocation failed - outputs");
    return 1;
}

static char faust_tilde_resize_ioputs(t_faust_tilde *x, int const nins, int const nouts)
{
    char valid = 0;
    t_sample** nsignals;
    gobj_vis((t_gobj *)x, x->f_canvas, 0);
    valid = faust_tilde_resize_inputs(x, nins) ? 1 : valid;
    valid = faust_tilde_resize_outputs(x, nouts) ? 1 : valid;
    nsignals = (t_sample **)resizebytes(x->f_signals,
                                        x->f_nsignals * sizeof(t_sample *),
                                        (x->f_noutlets + x->f_ninlets) * sizeof(t_sample *));
    
    
    gobj_vis((t_gobj *)x, x->f_canvas, 1);
    canvas_fixlinesfor(x->f_canvas, (t_text *)x);

    if(nsignals)
    {
        x->f_nsignals = x->f_noutlets + x->f_ninlets;
        x->f_signals  = nsignals;
        return valid;
    }
    pd_error(x, "faust~: memory allocation failed - signals");
    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                          FILE LOCALIZATION                                   //
//////////////////////////////////////////////////////////////////////////////////////////////////

static char* faust_tilde_get_dsp_file_path(t_faust_tilde *x)
{
    char* file = NULL;
    t_symbol const* name = x->f_dsp_name;
    t_symbol const* path;
    if(x->f_canvas)
    {
        path = canvas_getdir(x->f_canvas);
        if(path && path->s_name && name && name->s_name)
        {
            file = (char *)calloc(MAXFAUSTSTRING, sizeof(char *));
            if(file)
            {
                sprintf(file, "%s/%s.dsp", path->s_name, name->s_name);
                return file;
            }
            else
            {
                pd_error(x, "faust~: memory allocation failed");
            }
        }
        else
        {
            pd_error(x, "faust~: invalid canvas directory or DSP file name");
        }
    }
    else
    {
        pd_error(x, "faust~: invalid canvas");
    }
    return NULL;
}

static char* faust_tilde_get_default_include_path(t_faust_tilde *x)
{
    char* include_path;
    char const* path = class_gethelpdir(faust_tilde_class);
    if(path)
    {
        include_path = (char *)calloc(MAXFAUSTSTRING, sizeof(char));
        if(include_path)
        {
            sprintf(include_path, "%s/libs/", path);
            return include_path;
        }
        else
        {
            pd_error(x, "faust~: memory allocation failed - include path");
        }
    }
    else
    {
        pd_error(x, "faust~: cannot locate the include path");
    }
    return NULL;
}

static int faust_tilde_parse_compile_options(t_faust_tilde *x, int argc, t_atom* argv)
{
    int i = 0;
    char has_include = 0;
    for(i = 0; i < argc && i < MAXFAUSTOPTIONS; ++i)
    {
        x->f_compile_options[i] = (char *)calloc(MAXFAUSTSTRING, sizeof(char));
        if(x->f_compile_options[i])
        {
            if(argv[i].a_type == A_FLOAT)
            {
                sprintf(x->f_compile_options[i], "%i", (int)argv[i].a_w.w_float);
            }
            else if(argv[i].a_type == A_SYMBOL && argv[i].a_w.w_symbol)
            {
                sprintf(x->f_compile_options[i], "%s", argv[i].a_w.w_symbol->s_name);
                if(!strncmp(x->f_compile_options[i], "-I", 2))
                {
                    has_include = 1;
                }
            }
            else
            {
                pd_error(x, "faust~: option type invalid");
                memset(x->f_compile_options[i], 0, MAXFAUSTSTRING);
            }
            x->f_ncompile_options = i+1;
        }
        else
        {
            pd_error(x, "faust~: memory allocation failed - compile option");
            x->f_ncompile_options = i;
            return -1;
        }
    }
    if(!has_include)
    {
        x->f_compile_options[i] = (char *)calloc(MAXFAUSTSTRING, sizeof(char));
        if(x->f_compile_options[i])
        {
            sprintf(x->f_compile_options[i], "%s", "-I");
        }
        else
        {
            pd_error(x, "faust~: memory allocation failed - compile option");
            x->f_ncompile_options = i;
            return -1;
        }
        ++i;
        x->f_compile_options[i] = faust_tilde_get_default_include_path(x);
        if(!x->f_compile_options[i])
        {
            pd_error(x, "faust~: memory allocation failed - compile option");
            x->f_ncompile_options = i;
            return -1;
        }
        x->f_ncompile_options = i+1;
    }
    return 0;
}

static void faust_tilde_free_compile_options(t_faust_tilde *x)
{
    size_t i;
    if(x->f_compile_options)
    {
        for(i = 0; i < x->f_ncompile_options; ++i)
        {
            if(x->f_compile_options[i])
            {
                free(x->f_compile_options[i]);
            }
        }
    }
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                          FAUST INTERFACE                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_tilde_delete_instance(t_faust_tilde *x)
{
    if(x->f_dsp_instance)
    {
        deleteCDSPInstance(x->f_dsp_instance);
        x->f_dsp_instance = NULL;
    }
}

static void faust_tilde_delete_factory(t_faust_tilde *x)
{
    if(x->f_dsp_factory)
    {
        faust_tilde_delete_instance(x);
        deleteCDSPFactory(x->f_dsp_factory);
        x->f_dsp_factory = NULL;
    }
}

static void faust_tilde_ui_open_tab_box(t_faust_tilde* x, const char* label)
{
    //pd_error(x, "faust~: open tab box not supported yet (%s)", label);
}

static void faust_tilde_ui_open_horizontal_box(t_faust_tilde* x, const char* label)
{
    //pd_error(x, "faust~: open horizontal box not supported yet (%s)", label);
}

static void faust_tilde_ui_open_vertical_box(t_faust_tilde* x, const char* label)
{
    //pd_error(x, "faust~: open vertical box not supported yet (%s)", label);
}

static void faust_tilde_ui_close_box(t_faust_tilde* x)
{
    //pd_error(x, "faust~: close box not supported yet");
}



static void faust_tilde_delete_params(t_faust_tilde *x)
{
    if(x->f_params)
    {
        freebytes(x->f_params, x->f_nparams * sizeof(t_faust_param));
        x->f_params = NULL;
        x->f_nparams = 0;
    }
}

static void faust_tilde_save_params(t_faust_tilde *x)
{
    size_t i;
    for(i = 0; i < x->f_nparams; ++i)
    {
        x->f_params[i].p_saved = *x->f_params[i].p_zone;
    }
}

static void faust_tilde_restore_params(t_faust_tilde *x)
{
    size_t i;
    for(i = 0; i < x->f_nparams; ++i)
    {
        *x->f_params[i].p_zone = x->f_params[i].p_saved;
    }
}

static void faust_tilde_add_params(t_faust_tilde *x, const char* label, int const type, FAUSTFLOAT* zone,
                                   FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    char temp[MAXFAUSTSTRING];
    t_faust_param *newmemory;
    size_t size = x->f_nparams;
    if(x->f_params)
    {
        newmemory = (t_faust_param *)resizebytes(x->f_params, size * sizeof(t_faust_param), (size + 1) * sizeof(t_faust_param));
        if(!newmemory)
        {
            pd_error(x, "faust~: memory allocation failed - parameter");
            return;
        }
    }
    else
    {
        newmemory = (t_faust_param *)getbytes(sizeof(t_faust_param));
        if(!newmemory)
        {
            pd_error(x, "faust~: memory allocation failed - parameter");
            return;
        }
    }
    if(strnlen(label, MAXFAUSTSTRING) == 0 || label[0] == '0')
    {
        sprintf(temp, "param%i", (int)size+1);
        newmemory[size].p_label = gensym(temp);
        logpost(x, 3, "faust~: parameter has no label, empty string replace with '%s'", temp);
    }
    else
    {
        newmemory[size].p_label = gensym(label);
    }
    newmemory[size].p_type  = type;
    newmemory[size].p_zone  = zone;
    newmemory[size].p_min   = min;
    newmemory[size].p_max   = max;
    newmemory[size].p_step  = step;
    *newmemory[size].p_zone = init;
    x->f_params  = (t_faust_param *)newmemory;
    x->f_nparams++;
}

static void faust_tilde_ui_add_button(t_faust_tilde* x, const char* label, FAUSTFLOAT* zone)
{
    faust_tilde_add_params(x, label, 0, zone, 0, 0, 0, 0);
}

static void faust_tilde_ui_add_check_button(t_faust_tilde* x, const char* label, FAUSTFLOAT* zone)
{
    faust_tilde_add_params(x, label, 1, zone,0, 0, 1, 1);
}

static void faust_tilde_ui_add_vertical_slider(t_faust_tilde* x, const char* label, FAUSTFLOAT* zone,
                                               FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    faust_tilde_add_params(x, label, 2, zone, init, min, max, step);
}

static void faust_tilde_ui_add_horizontal_slider(t_faust_tilde* x, const char* label, FAUSTFLOAT* zone,
                                               FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    faust_tilde_add_params(x, label, 2, zone, init, min, max, step);
}

static void faust_tilde_ui_add_number_entry(t_faust_tilde* x, const char* label, FAUSTFLOAT* zone,
                                                 FAUSTFLOAT init, FAUSTFLOAT min, FAUSTFLOAT max, FAUSTFLOAT step)
{
    faust_tilde_add_params(x, label, 2, zone, init, min, max, step);
}




static void faust_tilde_ui_add_horizontal_bargraph(t_faust_tilde* x, const char* label,
                                                   FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
    pd_error(x, "faust~: add horizontal bargraph not supported yet");
}

static void faust_tilde_ui_add_vertical_bargraph(t_faust_tilde* x, const char* label,
                                                 FAUSTFLOAT* zone, FAUSTFLOAT min, FAUSTFLOAT max)
{
    pd_error(x, "faust~: add vertical bargraph not supported yet");
}

static void faust_tilde_ui_add_sound_file(t_faust_tilde* x, const char* label, const char* filename, struct Soundfile** sf_zone)
{
    pd_error(x, "faust~: add sound file not supported yet");
}

static void faust_tilde_ui_declare(t_faust_tilde* x, FAUSTFLOAT* zone, const char* key, const char* value)
{
    //pd_error(x, "faust~: ui declare %s: %s", key, value);
}




static void faust_tilde_meta_declare(t_faust_tilde* x, const char* key, const char* value)
{
    logpost(x, 3, "             %s: %s", key, value);
}

static void faust_tilde_print(t_faust_tilde *x)
{
    size_t i;
    startpost("faust~: compile options\n");
    for(i = 0; i < x->f_ncompile_options; i++)
    {
        poststring(x->f_compile_options[i]);
    }
    endpost();
}

static void faust_tilde_reload(t_faust_tilde *x)
{
    size_t i;
    char errors[MAXFAUSTSTRING];
    int dspstate = canvas_suspend_dsp();
    char* filepath = faust_tilde_get_dsp_file_path(x);
    if(filepath)
    {
        faust_tilde_delete_instance(x);
        faust_tilde_delete_factory(x);
        faust_tilde_delete_params(x);
        
        x->f_dsp_factory = createCDSPFactoryFromFile(filepath,
                                                     (int)x->f_ncompile_options, (const char**)x->f_compile_options,
                                                     "", errors, -1);
        if(strnlen(errors, MAXFAUSTSTRING))
        {
            pd_error(x, "faust~: try to load %s", filepath);
            for (i  = 0; i < x->f_ncompile_options; ++i)
            {
                if(x->f_compile_options[i])
                {
                    pd_error(x, "faust~: option %s", x->f_compile_options[i]);
                }
            }
            pd_error(x, "faust~: %s", errors);
            x->f_dsp_factory = NULL;
        }
        else
        {
            x->f_dsp_instance = createCDSPInstance(x->f_dsp_factory);
            if(x->f_dsp_instance)
            {
                if(!faust_tilde_resize_ioputs(x,
                                          getNumInputsCDSPInstance(x->f_dsp_instance),
                                          getNumOutputsCDSPInstance(x->f_dsp_instance)))
                {
                    logpost(x, 3, "\nfaust~: compilation from source '%s' succeeded", x->f_dsp_name->s_name);
                    metadataCDSPInstance(x->f_dsp_instance, &x->f_meta_glue);
                    buildUserInterfaceCDSPInstance(x->f_dsp_instance, &x->f_ui_glue);
                    
                    logpost(x, 3, "             %s: %i", "number of inputs",
                            getNumInputsCDSPInstance(x->f_dsp_instance));
                    logpost(x, 3, "             %s: %i", "number of outputs",
                            getNumOutputsCDSPInstance(x->f_dsp_instance));
                    for (i  = 0; i < x->f_nparams; ++i)
                    {
                        logpost(x, 3, "             parameter %i: %s", (int)i, x->f_params[i].p_label->s_name);
                    }
                }
                
            }
            else
            {
               pd_error(x, "faust~: memory allocation failed - instance");
            }
        }
        free(filepath);
    }
    else
    {
        pd_error(x, "faust~: source file not defined");
    }
    canvas_resume_dsp(dspstate);
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                  PURE DATA GENERIC INTERFACE                                 //
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_tilde_anything(t_faust_tilde *x, t_symbol* s, int argc, t_atom* argv)
{
    size_t i;
    t_float value;
    if(x->f_dsp_instance)
    {
        for(i = 0; i < x->f_nparams; ++i)
        {
            if(x->f_params[i].p_label == s)
            {
                if(x->f_params[i].p_type == 0)
                {
                    *x->f_params[i].p_zone = !(*x->f_params[i].p_zone != 0.f);
                    if(argc)
                    {
                        pd_error(x, "faust~: parameter '%s' two many arguments: 0 expected", s->s_name);
                    }
                }
                else if(x->f_params[i].p_type == 1)
                {
                    if(argc < 1 || argv[0].a_type != A_FLOAT)
                    {
                        pd_error(x, "faust~: parameter '%s' wrong arguments: 0 expected", s->s_name);
                        return;
                    }
                    *x->f_params[i].p_zone = argv[0].a_w.w_float != 0.f;
                    if(argc > 1)
                    {
                        pd_error(x, "faust~: parameter '%s' two many arguments: 1 float expected", s->s_name);
                    }
                }
                else if(x->f_params[i].p_type == 2)
                {
                    if(argc < 1 || argv[0].a_type != A_FLOAT)
                    {
                        pd_error(x, "faust~: parameter '%s' wrong arguments: 0 expected", s->s_name);
                        return;
                    }
                    value = argv[0].a_w.w_float;
                    value = value > x->f_params[i].p_min ? value : x->f_params[i].p_min;
                    value = value < x->f_params[i].p_max ? value : x->f_params[i].p_max;
                    *x->f_params[i].p_zone = value;
                    if(argc > 1)
                    {
                        pd_error(x, "faust~: parameter '%s' two many arguments: 1 float expected", s->s_name);
                    }
                }
                else
                {
                    pd_error(x, "faust~: wrong parameter '%s'", s->s_name);
                }
                return;
            }
        }
        pd_error(x, "faust~: parameter '%s' not defined", s->s_name);
    }
}

static void faust_tilde_bang(t_faust_tilde *x, t_float f)
{
    faust_tilde_anything(x, &s_, 0, NULL);
}

static void faust_tilde_symbol(t_faust_tilde *x, t_symbol* s)
{
    faust_tilde_anything(x, s, 0, NULL);
}

static void faust_tilde_list(t_faust_tilde *x, t_symbol* s, int argc, t_atom* argv)
{
    faust_tilde_anything(x, s, argc, argv);
}

static t_int *faust_tilde_perform(t_int *w)
{
    computeCDSPInstance((llvm_dsp *)w[1], (int)w[2], (float **)w[3], (float **)w[4]);
    return (w+5);
}

static char faust_tilde_is_valid(t_faust_tilde *x)
{
    size_t fnins, fnouts;
    if(!x->f_dsp_instance)
    {
        return -1;
    }
    fnins = (size_t)getNumInputsCDSPInstance(x->f_dsp_instance);
    fnouts = (size_t)getNumOutputsCDSPInstance(x->f_dsp_instance);
    if(fnins == 0)
        fnins = 1;
    if(fnins != x->f_ninlets)
    {
        pd_error(x, "faust~: number of inlets is invalid");
        return -1;
    }
    if(fnouts!= x->f_noutlets)
    {
        pd_error(x, "faust~: number of outlets is invalid");
        return -1;
    }
    if(x->f_nsignals != x->f_ninlets + x->f_ninlets)
    {
        pd_error(x, "faust~: number of signals is invalid");
        return -1;
    }
    if(!x->f_signals)
    {
        pd_error(x, "faust~: signals are not allocated");
        return -1;
    }
    return 0;
}

static void faust_tilde_dsp(t_faust_tilde *x, t_signal **sp)
{
    size_t i;
    size_t const ninlets = x->f_ninlets;
    size_t const noutlets = x->f_noutlets;
    if(!faust_tilde_is_valid(x))
    {
        faust_tilde_save_params(x);
        initCDSPInstance(x->f_dsp_instance, sp[0]->s_sr);
        for(i = 0; i < ninlets + noutlets; ++i)
        {
            x->f_signals[i] = sp[i]->s_vec;
        }
        dsp_add((t_perfroutine)faust_tilde_perform, 4,
                (t_int)x->f_dsp_instance, (t_int)sp[0]->s_n, (t_int)x->f_signals, (t_int)(x->f_signals+ninlets));
        faust_tilde_restore_params(x);
    }
}

static void faust_tilde_free(t_faust_tilde *x)
{
    faust_tilde_delete_instance(x);
    faust_tilde_delete_factory(x);
    faust_tilde_delete_params(x);
    faust_tilde_free_compile_options(x);
    faust_tilde_free_ioputs(x);
}

static void *faust_tilde_new(t_symbol* s, int argc, t_atom* argv)
{
    size_t i;
    t_faust_tilde* x = (t_faust_tilde *)pd_new(faust_tilde_class);
    if(x)
    {
        x->f_dsp_factory    = NULL;
        x->f_dsp_instance   = NULL;
        x->f_nsignals       = 0;
        x->f_signals        = NULL;
        x->f_f              = 0;
        
        x->f_meta_glue.metaInterface = x;
        x->f_meta_glue.declare       = (metaDeclareFun)faust_tilde_meta_declare;
        
        x->f_ui_glue.uiInterface            = x;
        x->f_ui_glue.openTabBox             = (openTabBoxFun)faust_tilde_ui_open_tab_box;
        x->f_ui_glue.openHorizontalBox      = (openHorizontalBoxFun)faust_tilde_ui_open_horizontal_box;
        x->f_ui_glue.openVerticalBox        = (openVerticalBoxFun)faust_tilde_ui_open_vertical_box;
        x->f_ui_glue.closeBox               = (closeBoxFun)faust_tilde_ui_close_box;
        
        x->f_ui_glue.addButton              = (addButtonFun)faust_tilde_ui_add_button;
        x->f_ui_glue.addCheckButton         = (addCheckButtonFun)faust_tilde_ui_add_check_button;
        x->f_ui_glue.addVerticalSlider      = (addVerticalSliderFun)faust_tilde_ui_add_vertical_slider;
        x->f_ui_glue.addHorizontalSlider    = (addHorizontalSliderFun)faust_tilde_ui_add_horizontal_slider;
        x->f_ui_glue.addNumEntry            = (addNumEntryFun)faust_tilde_ui_add_number_entry;
        
        x->f_ui_glue.addHorizontalBargraph  = (addHorizontalBargraphFun)faust_tilde_ui_add_horizontal_bargraph;
        x->f_ui_glue.addVerticalBargraph    = (addVerticalBargraphFun)faust_tilde_ui_add_vertical_bargraph;
        x->f_ui_glue.addSoundFile           = (addSoundFileFun)faust_tilde_ui_add_sound_file;
        x->f_ui_glue.declare                = (declareFun)faust_tilde_ui_declare;
        
        x->f_nparams        = 0;
        x->f_params         = NULL;
        
        x->f_canvas         = canvas_getcurrent();
        x->f_dsp_name       = atom_getsymbolarg(0, argc, argv);
        
        x->f_ncompile_options   = 0;
        for(i = 0; i < MAXFAUSTOPTIONS; ++i)
        {
            x->f_compile_options[i] = NULL;
        }
        x->f_ninlets            = 0;
        x->f_inlets             = NULL;
        x->f_noutlets           = 0;
        x->f_outlets            = NULL;
        if(argc == 0 || argv == NULL)
        {
            return x;
        }
        if(faust_tilde_parse_compile_options(x, argc-1, argv+1))
        {
            faust_tilde_free(x);
            return NULL;
        }
        faust_tilde_reload(x);
        if(!x->f_dsp_instance)
        {
            faust_tilde_free(x);
            return NULL;
        }
    }
    return x;
}

void faust_tilde_setup(void)
{
    t_class* c = class_new(gensym("faust~"),
                           (t_newmethod)faust_tilde_new, (t_method)faust_tilde_free,
                           sizeof(t_faust_tilde), CLASS_DEFAULT, A_GIMME, 0);
    if(c)
    {
        class_addmethod(c, (t_method)faust_tilde_dsp, gensym("dsp"), A_CANT);
        class_addmethod(c, (t_method)faust_tilde_reload, gensym("reload"), A_NULL);
        class_addmethod(c, (t_method)faust_tilde_print, gensym("print"), A_NULL);
        class_addbang(c, (t_method)faust_tilde_bang);
        class_addsymbol(c, (t_method)faust_tilde_symbol);
        class_addlist(c, (t_method)faust_tilde_list);
        class_addanything(c, (t_method)faust_tilde_anything);
        
        CLASS_MAINSIGNALIN(c, t_faust_tilde, f_f);
        
        logpost(NULL, 3, "Faust website: faust.grame.fr");
        logpost(NULL, 3, "Faust development: GRAME");
        
        logpost(NULL, 3, "faust~ compiler version: %s", getCLibFaustVersion());
        logpost(NULL, 3, "faust~ include directory: %s", class_gethelpdir(c));
        logpost(NULL, 3, "faust~ institutions: CICM - ANR MUSICOLL");
        logpost(NULL, 3, "faust~ external author: Pierre Guillot");
        logpost(NULL, 3, "faust~ website: github.com/grame-cncm/faust-pd");
    }
    
    faust_tilde_class = c;
}

