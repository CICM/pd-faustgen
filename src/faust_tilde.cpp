/*
// Copyright (c) 2018 Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <m_pd.h>
#include <faust/dsp/llvm-dsp.h>
#include <faust/dsp/interpreter-dsp.h>

typedef struct _faust_tilde
{
    t_object            f_obj;
    t_symbol*           f_name;
    llvm_dsp_factory*   f_dsp_factory;
    llvm_dsp*           f_dsp_instance;
} t_faust_tilde;

static t_class *faust_tilde_class;

static void faust_tilde_delete_factory(t_faust_tilde *x)
{
    if(x->f_dsp_factory)
    {
        deleteDSPFactory(x->f_dsp_factory);
        x->f_dsp_factory = NULL;
    }
}

static void faust_tilde_create_factory(t_faust_tilde *x)
{
    std::string errors;
    char const* argv[] = {"-llvm", "-interp"};
    faust_tilde_delete_factory(x);
    x->f_dsp_factory = createDSPFactoryFromString("", "", 2, argv, std::string(), errors);
    if(!errors.empty())
    {
        pd_error(x, "%s", errors.c_str());
    }
}

static void faust_tilde_dsp(t_faust_tilde *x, t_signal **sp)
{
    ;
}

static void faust_tilde_free(t_faust_tilde *x)
{
    faust_tilde_delete_factory(x);
}

static void *faust_tilde_new(t_symbol* s)
{
    t_faust_tilde* x = (t_faust_tilde *)pd_new(faust_tilde_class);
    if(x)
    {
        x->f_name           = s;
        x->f_dsp_factory    = NULL;
        x->f_dsp_instance   = NULL;
    }
    return x;
}

#ifdef __cplusplus
extern "C"
{
#endif
EXTERN void faust_tilde_setup(void)
{
    t_class* c = class_new(gensym("faust~"), (t_newmethod)faust_tilde_new, (t_method)faust_tilde_free, sizeof(t_faust_tilde), CLASS_NOINLET, A_SYMBOL, 0);
    if(c)
    {
        class_addmethod(c, (t_method)faust_tilde_dsp, gensym("dsp"), A_CANT);
    }
    faust_tilde_class = c;
}
#ifdef __cplusplus
}
#endif
