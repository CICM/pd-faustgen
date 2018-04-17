/*
// Copyright (c) 2016 Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#include <m_pd.h>
#include <faust/dsp/llvm-dsp.h>


typedef struct _faust_tilde
{
    t_object    f_obj;
    t_symbol*   f_name;
} t_faust_tilde;

static t_class *faust_tilde_class;

static void *faust_tilde_new(t_symbol* s)
{
    t_faust_tilde* x = (t_faust_tilde *)pd_new(faust_tilde_class);
    if(x)
    {
        x->f_name = s;
    }
    return x;
}

static void faust_tilde_dsp(t_faust_tilde *x, t_signal **sp)
{
    ;
}

#ifdef __cplusplus
extern "C"
{
#endif
EXTERN void faust_tilde_setup(void)
{
    t_class* c = class_new(gensym("faust~"), (t_newmethod)faust_tilde_new, (t_method)NULL, sizeof(t_faust_tilde), CLASS_NOINLET, A_SYMBOL, 0);
    if(c)
    {
        class_addmethod(c, (t_method)faust_tilde_dsp, gensym("dsp"), A_CANT);
    }
    faust_tilde_class = c;
}
#ifdef __cplusplus
}
#endif
