/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <m_pd.h>
#include <string.h>

#include <faust/dsp/llvm-c-dsp.h>
#include "faust_tilde_ui.h"
#include "faust_tilde_io.h"
#include "faust_tilde_options.h"

#define MAXFAUSTSTRING 4096
#define MAXFAUSTOPTIONS 128

typedef struct _faust_tilde
{
    t_object            f_obj;
    llvm_dsp_factory*   f_dsp_factory;
    llvm_dsp*           f_dsp_instance;
    t_faust_ui_manager* f_ui_manager;
    t_faust_io_manager* f_io_manager;
    t_faust_opt_manager* f_opt_manager;
 
    t_symbol*           f_dsp_name;
    t_float             f_dummy;
} t_faust_tilde;

static t_class *faust_tilde_class;


//////////////////////////////////////////////////////////////////////////////////////////////////
//                                          FAUST INTERFACE                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_tilde_delete_instance(t_faust_tilde *x)
{
    if(x->f_dsp_instance)
    {
        deleteCDSPInstance(x->f_dsp_instance);
    }
    x->f_dsp_instance = NULL;
}

static void faust_tilde_delete_factory(t_faust_tilde *x)
{
    faust_tilde_delete_instance(x);
    if(x->f_dsp_factory)
    {
        deleteCDSPFactory(x->f_dsp_factory);
    }
    x->f_dsp_factory = NULL;
}

static void faust_tilde_compile(t_faust_tilde *x)
{
    char const* filepath;
    int dspstate = canvas_suspend_dsp();
    if(!x->f_dsp_name)
    {
        return;
    }
    filepath = faust_opt_manager_get_full_path(x->f_opt_manager, x->f_dsp_name->s_name);
    if(filepath)
    {
        char errors[MAXFAUSTSTRING];
        int noptions            = (int)faust_opt_manager_get_noptions(x->f_opt_manager);
        char const** options    = faust_opt_manager_get_options(x->f_opt_manager);
        faust_tilde_delete_instance(x);
        faust_tilde_delete_factory(x);
        faust_ui_manager_clear(x->f_ui_manager);
        
        x->f_dsp_factory = createCDSPFactoryFromFile(filepath, noptions, options, "", errors, -1);
        if(strnlen(errors, MAXFAUSTSTRING))
        {
            pd_error(x, "faust~: try to load %s", filepath);
            pd_error(x, "faust~: %s", errors);
            x->f_dsp_factory = NULL;
            
            canvas_resume_dsp(dspstate);
            return;
        }

        x->f_dsp_instance = createCDSPInstance(x->f_dsp_factory);
        if(x->f_dsp_instance)
        {
            const int ninputs = getNumInputsCDSPInstance(x->f_dsp_instance);
            const int noutputs = getNumOutputsCDSPInstance(x->f_dsp_instance);
            logpost(x, 3, "\nfaust~: compilation from source '%s' succeeded", x->f_dsp_name->s_name);
            logpost(x, 3, "             %s: %i", "number of inputs", ninputs);
            logpost(x, 3, "             %s: %i", "number of outputs", noutputs);
            faust_ui_manager_init(x->f_ui_manager, x->f_dsp_instance);
            faust_io_manager_init(x->f_io_manager, ninputs, noutputs, faust_ui_manager_has_passive_ui(x->f_ui_manager));

            canvas_resume_dsp(dspstate);
            return;
        }
        
        pd_error(x, "faust~: memory allocation failed - instance");
        canvas_resume_dsp(dspstate);
        return;
    }
    pd_error(x, "faust~: source file not found %s", x->f_dsp_name->s_name);
    canvas_resume_dsp(dspstate);
}

static void faust_tilde_compile_options(t_faust_tilde *x, t_symbol* s, int argc, t_atom* argv)
{
    faust_opt_manager_parse_compile_options(x->f_opt_manager, argc, argv);
    faust_tilde_compile(x);
}

static void faust_tilde_read(t_faust_tilde *x, t_symbol* s)
{
    x->f_dsp_name = s;
    faust_tilde_compile(x);
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
            outlet_float(faust_io_manager_get_extra_output(x->f_io_manager), value);
            return;
        }
        pd_error(x, "faust~: ui glue '%s' not defined", s->s_name);
        return;
    }
    pd_error(x, "faust~: no dsp instance");
}

static t_int *faust_tilde_perform(t_int *w)
{
    computeCDSPInstance((llvm_dsp *)w[1], (int)w[2], (FAUSTFLOAT **)w[3], (FAUSTFLOAT **)w[4]);
    return (w+5);
}

static void faust_tilde_dsp(t_faust_tilde *x, t_signal **sp)
{
    if(x->f_dsp_instance && faust_io_manager_is_valid(x->f_io_manager))
    {
        faust_ui_manager_save_states(x->f_ui_manager);
        initCDSPInstance(x->f_dsp_instance, sp[0]->s_sr);
        faust_io_manager_prepare(x->f_io_manager, sp);
        dsp_add((t_perfroutine)faust_tilde_perform, 4,
                (t_int)x->f_dsp_instance, (t_int)sp[0]->s_n,
                (t_int)faust_io_manager_get_input_signals(x->f_io_manager),
                (t_int)faust_io_manager_get_output_signals(x->f_io_manager));
        faust_ui_manager_restore_states(x->f_ui_manager);
    }
}

static void faust_tilde_free(t_faust_tilde *x)
{
    faust_tilde_delete_instance(x);
    faust_tilde_delete_factory(x);
    faust_ui_manager_free(x->f_ui_manager);
    faust_io_manager_free(x->f_io_manager);
    faust_opt_manager_free(x->f_opt_manager);
}

static void *faust_tilde_new(t_symbol* s, int argc, t_atom* argv)
{
    t_faust_tilde* x = (t_faust_tilde *)pd_new(faust_tilde_class);
    if(x)
    {
        x->f_dsp_factory    = NULL;
        x->f_dsp_instance   = NULL;
        
        x->f_ui_manager     = faust_ui_manager_new((t_object *)x);
        x->f_io_manager     = faust_io_manager_new((t_object *)x, canvas_getcurrent());
        x->f_opt_manager    = faust_opt_manager_new((t_object *)x, canvas_getcurrent());
        x->f_dsp_name       = atom_getsymbolarg(0, argc, argv);
        faust_opt_manager_parse_compile_options(x->f_opt_manager, argc-1, argv+1);
        if(!argc)
        {
            return x;
        }
        faust_tilde_compile(x);
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
        class_addmethod(c,      (t_method)faust_tilde_dsp,              gensym("dsp"),            A_CANT);
        class_addmethod(c,      (t_method)faust_tilde_compile,          gensym("recompile"),      A_NULL);
        //class_addmethod(c,      (t_method)faust_tilde_read,             gensym("read"),           A_SYMBOL);
        class_addmethod(c,      (t_method)faust_tilde_compile_options,  gensym("compileoptions"), A_GIMME);
        class_addanything(c,    (t_method)faust_tilde_anything);
        
        CLASS_MAINSIGNALIN(c, t_faust_tilde, f_dummy);
        logpost(NULL, 3, "Faust website: faust.grame.fr");
        logpost(NULL, 3, "Faust development: GRAME");
        
        logpost(NULL, 3, "faust~ compiler version: %s", getCLibFaustVersion());
        logpost(NULL, 3, "faust~ include directory: %s", class_gethelpdir(c));
        logpost(NULL, 3, "faust~ institutions: CICM - ANR MUSICOLL");
        logpost(NULL, 3, "faust~ external author: Pierre Guillot");
        logpost(NULL, 3, "faust~ website: github.com/CICM/faust-pd");
    }
    
    faust_tilde_class = c;
}

