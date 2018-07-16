/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <m_pd.h>

#include <g_canvas.h>
#include <string.h>

#include <faust/dsp/llvm-c-dsp.h>
#include "faust_tilde_ui.h"

#define MAXFAUSTSTRING 4096
#define MAXFAUSTOPTIONS 128

typedef struct _faust_tilde
{
    t_object            f_obj;
    llvm_dsp_factory*   f_dsp_factory;
    llvm_dsp*           f_dsp_instance;
    size_t              f_nsignals;
    t_sample**          f_signals;
    t_float             f_f;
    
    MetaGlue            f_meta_glue;
    t_faust_ui_manager* f_ui_manager;
    
    t_canvas*           f_canvas;
    t_symbol*           f_dsp_name;
    
    size_t              f_ncompile_options;
    char*               f_compile_options[MAXFAUSTOPTIONS];
    
    size_t              f_ninlets;
    t_inlet**           f_inlets;
    size_t              f_noutlets;
    t_outlet**          f_outlets;
    char                f_allocated;
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
            noutlets[i] = outlet_new((t_object *)x, gensym("signal"));
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
    int const redraw = (x->f_canvas && glist_isvisible(x->f_canvas) && (!x->f_canvas->gl_isdeleting)
                  && glist_istoplevel(x->f_canvas) && x->f_allocated);
    if(redraw)
    {
        gobj_vis((t_gobj *)x, x->f_canvas, 0);
    }
    valid = faust_tilde_resize_inputs(x, nins) ? 1 : valid;
    valid = faust_tilde_resize_outputs(x, nouts) ? 1 : valid;
    nsignals = (t_sample **)resizebytes(x->f_signals,
                                        x->f_nsignals * sizeof(t_sample *),
                                        (x->f_noutlets + x->f_ninlets) * sizeof(t_sample *));
    
    if(nsignals)
    {
        x->f_nsignals = x->f_noutlets + x->f_ninlets;
        x->f_signals  = nsignals;
    }
    else
    {
        pd_error(x, "faust~: memory allocation failed - signals");
        valid = 1;
    }
    if(redraw)
    {
        gobj_vis((t_gobj *)x, x->f_canvas, 1);
        canvas_fixlinesfor(x->f_canvas, (t_text *)x);
    }
    return valid;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                          FILE LOCALIZATION                                   //
//////////////////////////////////////////////////////////////////////////////////////////////////

static char* faust_tilde_get_dsp_file_path(t_faust_tilde *x)
{
    if(x->f_canvas)
    {
        t_symbol const* name = x->f_dsp_name;
        t_symbol const* path = canvas_getdir(x->f_canvas);
        if(path && path->s_name && name && name->s_name)
        {
            char* file = (char *)calloc(MAXFAUSTSTRING, sizeof(char));
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
    char const* path = class_gethelpdir(faust_tilde_class);
    if(path)
    {
        char* include_path = (char *)calloc(MAXFAUSTSTRING, sizeof(char));
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
    if(x->f_compile_options)
    {
        size_t i;
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
    int dspstate = canvas_suspend_dsp();
    char* filepath = faust_tilde_get_dsp_file_path(x);
    if(filepath)
    {
        char errors[MAXFAUSTSTRING];
        faust_tilde_delete_instance(x);
        faust_tilde_delete_factory(x);
        faust_ui_manager_clear(x->f_ui_manager);
        
        x->f_dsp_factory = createCDSPFactoryFromFile(filepath,
                                                     (int)x->f_ncompile_options, (const char**)x->f_compile_options,
                                                     "", errors, -1);
        if(strnlen(errors, MAXFAUSTSTRING))
        {
            pd_error(x, "faust~: try to load %s", filepath);
            size_t i;
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
                faust_ui_manager_init(x->f_ui_manager, x->f_dsp_instance);
                logpost(x, 3, "\nfaust~: compilation from source '%s' succeeded", x->f_dsp_name->s_name);
                logpost(x, 3, "             %s: %i", "number of inputs",
                        getNumInputsCDSPInstance(x->f_dsp_instance));
                logpost(x, 3, "             %s: %i", "number of outputs",
                        getNumOutputsCDSPInstance(x->f_dsp_instance));
                if(!faust_tilde_resize_ioputs(x,
                                          getNumInputsCDSPInstance(x->f_dsp_instance),
                                          getNumOutputsCDSPInstance(x->f_dsp_instance)))
                {
                    metadataCDSPInstance(x->f_dsp_instance, &x->f_meta_glue);
                    
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
    if(x->f_dsp_instance)
    {
        t_float value;
        if(!faust_ui_manager_set(x->f_ui_manager, s, atom_getfloatarg(0, argc, argv)))
        {
            return;
        }
        if(!faust_ui_manager_get(x->f_ui_manager, s, &value))
        {
            outlet_float(NULL, value);
            return;
        }
        pd_error(x, "faust~: parameter '%s' not defined", s->s_name);
    }
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
    if(!faust_tilde_is_valid(x))
    {
        size_t i;
        size_t const ninlets = x->f_ninlets;
        size_t const noutlets = x->f_noutlets;
        faust_ui_manager_save_states(x->f_ui_manager);
        initCDSPInstance(x->f_dsp_instance, sp[0]->s_sr);
        for(i = 0; i < ninlets + noutlets; ++i)
        {
            x->f_signals[i] = sp[i]->s_vec;
        }
        dsp_add((t_perfroutine)faust_tilde_perform, 4,
                (t_int)x->f_dsp_instance, (t_int)sp[0]->s_n, (t_int)x->f_signals, (t_int)(x->f_signals+ninlets));
        faust_ui_manager_restore_states(x->f_ui_manager);
    }
}

static void faust_tilde_free(t_faust_tilde *x)
{
    faust_tilde_delete_instance(x);
    faust_tilde_delete_factory(x);
    faust_ui_manager_free(x->f_ui_manager);
    faust_tilde_free_compile_options(x);
    faust_tilde_free_ioputs(x);
}

static void *faust_tilde_new(t_symbol* s, int argc, t_atom* argv)
{
    t_faust_tilde* x = (t_faust_tilde *)pd_new(faust_tilde_class);
    if(x)
    {
        size_t i;
        x->f_dsp_factory    = NULL;
        x->f_dsp_instance   = NULL;
        x->f_nsignals       = 0;
        x->f_signals        = NULL;
        x->f_f              = 0;
        
        x->f_ui_manager     = faust_ui_manager_new((t_object *)x);
        x->f_meta_glue.metaInterface = x;
        x->f_meta_glue.declare       = (metaDeclareFun)faust_tilde_meta_declare;
        
        x->f_canvas         = canvas_getcurrent();
        x->f_dsp_name       = atom_getsymbolarg(0, argc, argv);
        
        x->f_ncompile_options   = 0;
        for(i = 0; i < MAXFAUSTOPTIONS; ++i)
        {
            x->f_compile_options[i] = NULL;
        }
        x->f_ninlets            = 1;
        x->f_inlets             = NULL;
        x->f_noutlets           = 0;
        x->f_outlets            = NULL;
        x->f_allocated          = 0;
        
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
        x->f_allocated = 1;
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
        class_addmethod(c,      (t_method)faust_tilde_dsp,      gensym("dsp"),      A_CANT);
        class_addmethod(c,      (t_method)faust_tilde_reload,   gensym("reload"),   A_NULL);
        class_addmethod(c,      (t_method)faust_tilde_print,    gensym("print"),    A_NULL);
        class_addanything(c,    (t_method)faust_tilde_anything);
        
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

