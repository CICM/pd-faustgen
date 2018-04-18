/*
// Copyright (c) 2018 Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <m_pd.h>
#include <string.h>
#include <faust/dsp/llvm-c-dsp.h>

typedef struct _faust_tilde
{
    t_object            f_obj;
    llvm_dsp_factory*   f_dsp_factory;
    llvm_dsp*           f_dsp_instance;
    
    t_symbol*           f_dsp_name;
    size_t              f_filepath_size;
    char*               f_filepath;
    size_t              f_compile_option_size;
    char**              f_compile_option;
    size_t              f_include_option_size;
    char*               f_include_option;
} t_faust_tilde;

static t_class *faust_tilde_class;

static void faust_tilde_delete_factory(t_faust_tilde *x)
{
    if(x->f_dsp_factory)
    {
        deleteCDSPFactory(x->f_dsp_factory);
        x->f_dsp_factory = NULL;
    }
}

static void faust_tilde_delete_instance(t_faust_tilde *x)
{
    if(x->f_dsp_instance)
    {
        deleteCDSPInstance(x->f_dsp_instance);
        x->f_dsp_instance = NULL;
    }
}

static void faust_tilde_reload(t_faust_tilde *x)
{
    char errors[4096];
    char const* argv[] = {"-I", x->f_include_option};
    int dspstate = canvas_suspend_dsp();
    if(x->f_filepath)
    {
        faust_tilde_delete_factory(x);
        x->f_dsp_factory = createCDSPFactoryFromFile(x->f_filepath, 2, argv, "", errors, -1);
        if(strnlen(errors, 4096))
        {
            pd_error(x, "faust~: %s", errors);
        }
        else
        {
            faust_tilde_delete_instance(x);
            x->f_dsp_instance = createCDSPInstance(x->f_dsp_factory);
            logpost(x, 3, "faust~: dsp %s compiled", x->f_dsp_name->s_name);
        }
    }
    else
    {
        pd_error(x, "faust~: DSP file not defined");
    }
    canvas_resume_dsp(dspstate);
}

static void faust_tilde_get_dsp_file(t_faust_tilde *x)
{
    t_symbol const* file = x->f_dsp_name;
    t_symbol const* path = canvas_getcurrentdir();
    if(path && path->s_name && file && file->s_name)
    {
        x->f_filepath_size = strnlen(path->s_name, 4096) + strnlen(file->s_name, 4096) + 3;
        x->f_filepath = (char *)getbytes(x->f_filepath_size);
        if(x->f_filepath)
        {
            sprintf(x->f_filepath, "%s/%s.dsp", path->s_name, file->s_name);
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

static void faust_tilde_get_include_path(t_faust_tilde *x)
{
    char const* path = class_gethelpdir(faust_tilde_class);
    if(path)
    {
        x->f_include_option_size = strnlen(path, 4096) + strnlen("/libs", 5);
        x->f_include_option = (char *)getbytes(x->f_include_option_size);
        if(x->f_include_option)
        {
            sprintf(x->f_include_option, "%s/libs", path);
        }
        else
        {
            pd_error(x, "faust~: memory allocation failed");
        }
    }
    else
    {
        pd_error(x, "faust~: cannot locate the include path");
    }
}

static void faust_tilde_dsp(t_faust_tilde *x, t_signal **sp)
{
    ;
}


static void faust_tilde_free(t_faust_tilde *x)
{
    faust_tilde_delete_instance(x);
    faust_tilde_delete_factory(x);
    if(x->f_filepath)
    {
        freebytes(x->f_filepath, x->f_filepath_size);
    }
}

static void *faust_tilde_new(t_symbol* s)
{
    t_faust_tilde* x = (t_faust_tilde *)pd_new(faust_tilde_class);
    if(x)
    {
        x->f_dsp_factory    = NULL;
        x->f_dsp_instance   = NULL;
        x->f_dsp_name       = s;
        x->f_filepath_size  = 0;
        x->f_filepath       = NULL;
        faust_tilde_get_dsp_file(x);
        faust_tilde_get_include_path(x);
        faust_tilde_reload(x);
    }
    return x;
}

extern "C" void faust_tilde_setup(void)
{
    t_class* c = class_new(gensym("faust~"),
                           (t_newmethod)faust_tilde_new, (t_method)faust_tilde_free,
                           sizeof(t_faust_tilde), CLASS_DEFAULT, A_SYMBOL, 0);
    if(c)
    {
        class_addmethod(c, (t_method)faust_tilde_dsp, gensym("dsp"), A_CANT);
        class_addmethod(c, (t_method)faust_tilde_reload, gensym("reload"), A_NULL);
    }
    post("faust~ compiler version: %s", getCLibFaustVersion());
    faust_tilde_class = c;
}

