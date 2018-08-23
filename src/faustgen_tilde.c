/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <m_pd.h>
#include <string.h>
#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <sys/stat.h>

#define FAUSTFLOAT t_sample
#include <faust/dsp/llvm-c-dsp.h>

#include "faust_tilde_ui.h"
#include "faust_tilde_io.h"
#include "faust_tilde_options.h"

#define MAXFAUSTSTRING 4096
#define MAXFAUSTOPTIONS 128

typedef struct _faustgen_tilde
{
    t_object            f_obj;
    llvm_dsp_factory*   f_dsp_factory;
    llvm_dsp*           f_dsp_instance;
    
    float**             f_signal_matrix_single;
    float*              f_signal_aligned_single;
    
    double**            f_signal_matrix_double;
    double*             f_signal_aligned_double;
    
    t_faust_ui_manager* f_ui_manager;
    t_faust_io_manager* f_io_manager;
    t_faust_opt_manager* f_opt_manager;
 
    t_symbol*           f_dsp_name;
    t_float             f_dummy;
    t_clock*            f_clock;
    double              f_clock_time;
    long                f_time;
} t_faustgen_tilde;

static t_class *faustgen_tilde_class;


//////////////////////////////////////////////////////////////////////////////////////////////////
//                                          FAUST INTERFACE                                     //
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faustgen_tilde_delete_instance(t_faustgen_tilde *x)
{
    if(x->f_dsp_instance)
    {
        deleteCDSPInstance(x->f_dsp_instance);
    }
    x->f_dsp_instance = NULL;
}

static void faustgen_tilde_delete_factory(t_faustgen_tilde *x)
{
    faustgen_tilde_delete_instance(x);
    if(x->f_dsp_factory)
    {
        deleteCDSPFactory(x->f_dsp_factory);
    }
    x->f_dsp_factory = NULL;
}

static void faustgen_tilde_compile(t_faustgen_tilde *x)
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
        llvm_dsp* instance;
        llvm_dsp_factory* factory;
        char errors[MAXFAUSTSTRING];
        int noptions            = (int)faust_opt_manager_get_noptions(x->f_opt_manager);
        char const** options    = faust_opt_manager_get_options(x->f_opt_manager);
        
        factory = createCDSPFactoryFromFile(filepath, noptions, options, "", errors, -1);
        if(strnlen(errors, MAXFAUSTSTRING))
        {
            pd_error(x, "faustgen~: try to load %s", filepath);
            pd_error(x, "faustgen~: %s", errors);
            faustgen_tilde_delete_instance(x);
            faustgen_tilde_delete_factory(x);
            canvas_resume_dsp(dspstate);
            return;
        }
        
        
        instance = createCDSPInstance(factory);
        if(instance)
        {
            const int ninputs = getNumInputsCDSPInstance(instance);
            const int noutputs = getNumOutputsCDSPInstance(instance);
            logpost(x, 3, "\nfaustgen~: compilation from source '%s' succeeded", x->f_dsp_name->s_name);
            faust_ui_manager_init(x->f_ui_manager, instance);
            faust_io_manager_init(x->f_io_manager, ninputs, noutputs);
            
            faustgen_tilde_delete_instance(x);
            faustgen_tilde_delete_factory(x);
            
            x->f_dsp_factory  = factory;
            x->f_dsp_instance = instance;
            canvas_resume_dsp(dspstate);
            return;
        }
        
        faustgen_tilde_delete_instance(x);
        faustgen_tilde_delete_factory(x);
        pd_error(x, "faustgen~: memory allocation failed - instance");
        canvas_resume_dsp(dspstate);
        return;
    }
    pd_error(x, "faustgen~: source file not found %s", x->f_dsp_name->s_name);
    canvas_resume_dsp(dspstate);
}

static void faustgen_tilde_compile_options(t_faustgen_tilde *x, t_symbol* s, int argc, t_atom* argv)
{
    faust_opt_manager_parse_compile_options(x->f_opt_manager, argc, argv);
    faustgen_tilde_compile(x);
}

static void faustgen_tilde_open_texteditor(t_faustgen_tilde *x)
{
    if(x->f_dsp_instance)
    {
        char message[MAXPDSTRING];
#ifdef _WIN32
        sys_bashfilename(faust_opt_manager_get_full_path(x->f_opt_manager, x->f_dsp_name->s_name), message);
#elif __APPLE__
        sprintf(message, "open -t %s", faust_opt_manager_get_full_path(x->f_opt_manager, x->f_dsp_name->s_name));
#else
        sprintf(message, "xdg-open %s", faust_opt_manager_get_full_path(x->f_opt_manager, x->f_dsp_name->s_name));
#endif
        system(message);
        return;
    }
    pd_error(x, "faustgen~: no FAUST DSP file defined");
}

/*
static void faustgen_tilde_read(t_faustgen_tilde *x, t_symbol* s)
{
    x->f_dsp_name = s;
    faustgen_tilde_compile(x);
}
 */

static long faustgen_tilde_get_time(t_faustgen_tilde *x)
{
    if(x->f_dsp_instance)
    {
        struct stat attrib;
        stat(faust_opt_manager_get_full_path(x->f_opt_manager, x->f_dsp_name->s_name), &attrib);
        return attrib.st_ctime;
    }
    return 0;
}

static void faustgen_tilde_autocompile_tick(t_faustgen_tilde *x)
{
    long ntime = faustgen_tilde_get_time(x);
    if(ntime != x->f_time)
    {
        x->f_time = ntime;
        faustgen_tilde_compile(x);
    }
    clock_delay(x->f_clock, x->f_clock_time);
}

static void faustgen_tilde_autocompile(t_faustgen_tilde *x, t_symbol* s, int argc, t_atom* argv)
{
    float state = atom_getfloatarg(0, argc, argv);
    if(fabsf(state) > FLT_EPSILON)
    {
        float time = atom_getfloatarg(1, argc, argv);
        x->f_clock_time = (time > FLT_EPSILON) ? (double)time : 100.;
        x->f_time = faustgen_tilde_get_time(x);
        clock_delay(x->f_clock, x->f_clock_time);
    }
    else
    {
        clock_unset(x->f_clock);
    }
}

static void faustgen_tilde_print(t_faustgen_tilde *x)
{
    if(x->f_dsp_factory)
    {
        post("faustgen~: %s", faust_opt_manager_get_full_path(x->f_opt_manager, x->f_dsp_name->s_name));
        faust_io_manager_print(x->f_io_manager, 0);
        faust_opt_manager_print(x->f_opt_manager, 0);
        faust_ui_manager_print(x->f_ui_manager, 0);
    }
    else
    {
        pd_error(x, "faustgen~: no FAUST DSP file defined");
    }
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//                                  PURE DATA GENERIC INTERFACE                                 //
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faustgen_tilde_anything(t_faustgen_tilde *x, t_symbol* s, int argc, t_atom* argv)
{
    if(x->f_dsp_instance)
    {
        if(!argc)
        {
            t_float value;
            if(!faust_ui_manager_get_value(x->f_ui_manager, s, &value))
            {
                t_atom av;
                SETFLOAT(&av, value);
                outlet_anything(faust_io_manager_get_extra_output(x->f_io_manager), s, 1, &av);
                return;
            }
            pd_error(x, "faustgen~: parameter '%s' not defined", s->s_name);
            return;
        }
        else if(argc == 1)
        {
            if(argv[0].a_type != A_FLOAT)
            {
                pd_error(x, "faustgen~: parameter requires a float value");
                return;
            }
            if(!faust_ui_manager_set_value(x->f_ui_manager, s, argv[0].a_w.w_float))
            {
                return;
            }
            pd_error(x, "faustgen~: parameter '%s' not defined", s->s_name);
            return;
        }
        else
        {
            int i, start;
            char name[MAXFAUSTSTRING];
            if(argv[0].a_type != A_FLOAT)
            {
                pd_error(x, "faustgen~: list parameters requires a first index");
                return;
            }
            start = (int)argv[0].a_w.w_float;
            for(i = 0; i < argc - 1; ++i)
            {
                if(start+i < 10)
                {
                    sprintf(name, "%s  %i", s->s_name, start+i);
                }
                else if(start+i < 100)
                {
                    sprintf(name, "%s %i", s->s_name, start+i);
                }
                else
                {
                    sprintf(name, "%s%i", s->s_name, start+i);
                }
                if(argv[i+1].a_type != A_FLOAT)
                {
                    pd_error(x, "faustgen~: active parameter requires a float value");
                }
                if(faust_ui_manager_set_value(x->f_ui_manager, gensym(name), argv[i+1].a_w.w_float))
                {
                    pd_error(x, "faustgen~: active parameter '%s' not defined", name);
                    return;
                }
            }
            return;
        }
    }
    pd_error(x, "faustgen~: no dsp instance");
}

static t_int *faustgen_tilde_perform_single(t_int *w)
{
    int i, j;
    llvm_dsp *dsp = (llvm_dsp *)w[1];
    int const nsamples  = (int)w[2];
    int const ninputs   = (int)w[3];
    int const noutputs  = (int)w[4];
    float** faustsigs   = (float **)w[5];
    t_sample const** realinputs = (t_sample const**)w[6];
    t_sample** realoutputs      = (t_sample **)w[7];
    for(i = 0; i < ninputs; ++i)
    {
        for(j = 0; j < nsamples; ++j)
        {
            faustsigs[i][j] = (FAUSTFLOAT)realinputs[i][j];
        }
    }
    computeCDSPInstance(dsp, nsamples, (FAUSTFLOAT**)faustsigs, (FAUSTFLOAT**)(faustsigs+ninputs));
    for(i = 0; i < noutputs; ++i)
    {
        for(j = 0; j < nsamples; ++j)
        {
            realoutputs[i][j] = (t_sample)faustsigs[ninputs+i][j];
        }
    }
    return (w+8);
}

static t_int *faustgen_tilde_perform_double(t_int *w)
{
    int i, j;
    llvm_dsp *dsp = (llvm_dsp *)w[1];
    int const nsamples  = (int)w[2];
    int const ninputs   = (int)w[3];
    int const noutputs  = (int)w[4];
    double** faustsigs  = (double **)w[5];
    t_sample const** realinputs = (t_sample const**)w[6];
    t_sample** realoutputs      = (t_sample **)w[7];
    for(i = 0; i < ninputs; ++i)
    {
        for(j = 0; j < nsamples; ++j)
        {
            faustsigs[i][j] = (FAUSTFLOAT)realinputs[i][j];
        }
    }
    computeCDSPInstance(dsp, nsamples, (FAUSTFLOAT**)faustsigs, (FAUSTFLOAT**)(faustsigs+ninputs));
    for(i = 0; i < noutputs; ++i)
    {
        for(j = 0; j < nsamples; ++j)
        {
            realoutputs[i][j] = (t_sample)faustsigs[ninputs+i][j];
        }
    }
    return (w+8);
}

static void faustgen_tilde_free_signals(t_faustgen_tilde *x)
{
    if(x->f_signal_aligned_single)
    {
        free(x->f_signal_aligned_single);
    }
    x->f_signal_aligned_single = NULL;
    if(x->f_signal_matrix_single)
    {
        free(x->f_signal_matrix_single);
    }
    x->f_signal_matrix_single = NULL;
    
    if(x->f_signal_aligned_double)
    {
        free(x->f_signal_aligned_double);
    }
    x->f_signal_aligned_double = NULL;
    if(x->f_signal_matrix_double)
    {
        free(x->f_signal_matrix_double);
    }
    x->f_signal_matrix_double = NULL;
}

static void faustgen_tilde_alloc_signals_single(t_faustgen_tilde *x, size_t const ninputs, size_t const noutputs, size_t const nsamples)
{
    size_t i;
    faustgen_tilde_free_signals(x);
    x->f_signal_aligned_single = (float *)malloc((ninputs + noutputs) * nsamples * sizeof(float));
    if(!x->f_signal_aligned_single)
    {
        pd_error(x, "memory allocation failed");
        return;
    }
    x->f_signal_matrix_single = (float **)malloc((ninputs + noutputs) * sizeof(float *));
    if(!x->f_signal_matrix_single)
    {
        pd_error(x, "memory allocation failed");
        return;
    }
    for(i = 0; i < (ninputs + noutputs); ++i)
    {
        x->f_signal_matrix_single[i] = (x->f_signal_aligned_single+(i*nsamples));
    }
}

static void faustgen_tilde_alloc_signals_double(t_faustgen_tilde *x, size_t const ninputs, size_t const noutputs, size_t const nsamples)
{
    size_t i;
    faustgen_tilde_free_signals(x);
    x->f_signal_aligned_double = (double *)malloc((ninputs + noutputs) * nsamples * sizeof(double));
    if(!x->f_signal_aligned_double)
    {
        pd_error(x, "memory allocation failed");
        return;
    }
    x->f_signal_matrix_double = (double **)malloc((ninputs + noutputs) * sizeof(double *));
    if(!x->f_signal_matrix_double)
    {
        pd_error(x, "memory allocation failed");
        return;
    }
    for(i = 0; i < (ninputs + noutputs); ++i)
    {
        x->f_signal_matrix_double[i] = (x->f_signal_aligned_double+(i*nsamples));
    }
}

static void faustgen_tilde_dsp(t_faustgen_tilde *x, t_signal **sp)
{
    if(x->f_dsp_instance)
    {
        char initialized = getSampleRateCDSPInstance(x->f_dsp_instance) != sp[0]->s_sr;
        if(initialized)
        {
            faust_ui_manager_save_states(x->f_ui_manager);
            initCDSPInstance(x->f_dsp_instance, sp[0]->s_sr);
        }
        if(!faust_io_manager_prepare(x->f_io_manager, sp))
        {
            size_t const ninputs  = faust_io_manager_get_ninputs(x->f_io_manager);
            size_t const noutputs = faust_io_manager_get_noutputs(x->f_io_manager);
            size_t const nsamples = (size_t)sp[0]->s_n;

            if(faust_opt_has_double_precision(x->f_opt_manager))
            {
                faustgen_tilde_alloc_signals_double(x, ninputs, noutputs, nsamples);
                dsp_add((t_perfroutine)faustgen_tilde_perform_double, 7,
                        (t_int)x->f_dsp_instance, (t_int)nsamples, (t_int)ninputs, (t_int)noutputs,
                        (t_int)x->f_signal_matrix_double,
                        (t_int)faust_io_manager_get_input_signals(x->f_io_manager),
                        (t_int)faust_io_manager_get_output_signals(x->f_io_manager));
            }
            else
            {
                faustgen_tilde_alloc_signals_single(x, ninputs, noutputs, nsamples);
                dsp_add((t_perfroutine)faustgen_tilde_perform_single, 7,
                        (t_int)x->f_dsp_instance, (t_int)nsamples, (t_int)ninputs, (t_int)noutputs,
                        (t_int)x->f_signal_matrix_single,
                        (t_int)faust_io_manager_get_input_signals(x->f_io_manager),
                        (t_int)faust_io_manager_get_output_signals(x->f_io_manager));
            }
        }
        if(initialized)
        {
            faust_ui_manager_restore_states(x->f_ui_manager);
        }
    }
}

static void faustgen_tilde_free(t_faustgen_tilde *x)
{
    faustgen_tilde_delete_instance(x);
    faustgen_tilde_delete_factory(x);
    faust_ui_manager_free(x->f_ui_manager);
    faust_io_manager_free(x->f_io_manager);
    faust_opt_manager_free(x->f_opt_manager);
    faustgen_tilde_free_signals(x);
}

static void *faustgen_tilde_new(t_symbol* s, int argc, t_atom* argv)
{
    t_faustgen_tilde* x = (t_faustgen_tilde *)pd_new(faustgen_tilde_class);
    if(x)
    {
        char default_file[MAXPDSTRING];
        sprintf(default_file, "%s/.default", class_gethelpdir(faustgen_tilde_class));
        x->f_dsp_factory    = NULL;
        x->f_dsp_instance   = NULL;
        
        x->f_signal_matrix_single  = NULL;
        x->f_signal_aligned_single = NULL;
        x->f_signal_matrix_double  = NULL;
        x->f_signal_aligned_double = NULL;
        
        x->f_ui_manager     = faust_ui_manager_new((t_object *)x);
        x->f_io_manager     = faust_io_manager_new((t_object *)x, canvas_getcurrent());
        x->f_opt_manager    = faust_opt_manager_new((t_object *)x, canvas_getcurrent());
        x->f_dsp_name       = argc ? atom_getsymbolarg(0, argc, argv) : gensym(default_file);
        x->f_clock          = clock_new(x, (t_method)faustgen_tilde_autocompile_tick);
        faust_opt_manager_parse_compile_options(x->f_opt_manager, argc ? argc-1 : 0, argv ? argv+1 : NULL);
        faustgen_tilde_compile(x);
        if(!x->f_dsp_instance)
        {
            faustgen_tilde_free(x);
            return NULL;
        }
    }
    return x;
}

void faustgen_tilde_setup(void)
{
    t_class* c = class_new(gensym("faustgen~"),
                           (t_newmethod)faustgen_tilde_new, (t_method)faustgen_tilde_free,
                           sizeof(t_faustgen_tilde), CLASS_DEFAULT, A_GIMME, 0);
    
    if(c)
    {
        class_addmethod(c,  (t_method)faustgen_tilde_dsp,               gensym("dsp"),              A_CANT, 0);
        class_addmethod(c,  (t_method)faustgen_tilde_compile,           gensym("compile"),          A_NULL, 0);
        class_addmethod(c,  (t_method)faustgen_tilde_compile_options,   gensym("compileoptions"),   A_GIMME, 0);
        class_addmethod(c,  (t_method)faustgen_tilde_autocompile,       gensym("autocompile"),      A_GIMME, 0);
        class_addmethod(c,  (t_method)faustgen_tilde_print,             gensym("print"),            A_NULL, 0);
        class_addmethod(c,  (t_method)faustgen_tilde_open_texteditor,   gensym("click"),            A_NULL, 0);
        
        //class_addmethod(c,      (t_method)faustgen_tilde_read,             gensym("read"),           A_SYMBOL);
        class_addanything(c, (t_method)faustgen_tilde_anything);
        
        CLASS_MAINSIGNALIN(c, t_faustgen_tilde, f_dummy);
        logpost(NULL, 3, "Faust website: faust.grame.fr");
        logpost(NULL, 3, "Faust development: GRAME");
        
        logpost(NULL, 3, "faustgen~ compiler version: %s", getCLibFaustVersion());
        logpost(NULL, 3, "faustgen~ default include directory: %s", class_gethelpdir(c));
        logpost(NULL, 3, "faustgen~ institutions: CICM - ANR MUSICOLL");
        logpost(NULL, 3, "faustgen~ external author: Pierre Guillot");
        logpost(NULL, 3, "faustgen~ website: github.com/CICM/pd-faustgen");
    }
    
    faustgen_tilde_class = c;
}

