/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/


#include "faust_tilde_io.h"
#include <g_canvas.h>
#include <assert.h>

typedef struct _faust_io_manager
{
    t_object*   f_owner;
    t_canvas*   f_canvas;
    
    size_t      f_nsignals;
    t_sample**  f_signals;
    
    size_t      f_ninlets;
    t_inlet**   f_inlets;
    
    size_t      f_noutlets;
    t_outlet**  f_outlets;
    t_outlet*   f_extra_outlet;
    
    char        f_valid;
}t_faust_io_manager;

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                      PURE DATA IO DYNAMIC                                    //
//////////////////////////////////////////////////////////////////////////////////////////////////


// FREE METHODS
//////////////////////////////////////////////////////////////////////////////////////////////////
static void faust_io_manager_free_signals(t_faust_io_manager *x)
{
    if(x->f_signals)
    {
        freebytes(x->f_signals, sizeof(t_sample*) * x->f_nsignals);
    }
    x->f_signals  = NULL;
    x->f_nsignals = 0;
}

static void faust_io_manager_free_inputs(t_faust_io_manager *x)
{
    if(x->f_inlets && x->f_ninlets)
    {
        freebytes(x->f_inlets, sizeof(t_inlet*) * x->f_ninlets);
    }
    x->f_inlets  = NULL;
    x->f_ninlets = 0;
}

static void faust_io_manager_free_outputs(t_faust_io_manager *x)
{
    if(x->f_outlets && x->f_noutlets)
    {
        freebytes(x->f_outlets, sizeof(t_outlet*) * x->f_noutlets);
    }
    x->f_outlets  = NULL;
    x->f_noutlets = 0;
}

// ALLOCATE-RESIZE METHODS
//////////////////////////////////////////////////////////////////////////////////////////////////

static char faust_io_manager_resize_signals(t_faust_io_manager *x, size_t nsignals)
{
    t_sample** nsigs;
    if(x->f_nsignals == nsignals)
    {
        return 0;
    }
    nsigs = (t_sample **)resizebytes(x->f_signals, x->f_nsignals * sizeof(t_sample*), nsignals * sizeof(t_sample*));
    if(nsigs)
    {
        x->f_nsignals = nsignals;
        x->f_signals  = nsigs;
        return 0;
    }
    pd_error(x->f_owner, "faustgen~: memory allocation failed - signals");
    return 4;
}

static char faust_io_manager_resize_inputs(t_faust_io_manager *x, size_t const nins)
{
    t_inlet** ninlets;
    size_t i;
    size_t const cins = faust_io_manager_get_ninputs(x);
    size_t const rnins = nins;
    if(rnins == cins)
    {
        return 0;
    }
    for(i = cins; i > rnins; --i)
    {
        inlet_free(x->f_inlets[i-1]);
        x->f_inlets[i-1] = NULL;
    }
    ninlets = (t_inlet **)resizebytes(x->f_inlets, sizeof(t_inlet*) * cins, sizeof(t_inlet*) * rnins);
    if(ninlets)
    {
        for(i = cins; i < rnins; ++i)
        {
            ninlets[i] = signalinlet_new((t_object *)x->f_owner, 0);
            if(!ninlets[i])
            {
                pd_error(x->f_owner, "faustgen~: memory allocation failed - input %i", (int)i);
            }
        }
        x->f_inlets = ninlets;
        x->f_ninlets = rnins;
        return 0;
    }
    pd_error(x->f_owner, "faustgen~: memory allocation failed - inputs");
    return 2;
}

static char faust_io_manager_resize_outputs(t_faust_io_manager *x, size_t const nouts, char const extraout)
{
    t_outlet** noutlets;
    size_t i;
    size_t const couts = faust_io_manager_get_noutputs(x);
    size_t const rnouts = nouts;
    
    if(rnouts == couts)
    {
        if(faust_io_manager_has_extra_output(x) && !extraout)
        {
            outlet_free(x->f_extra_outlet);
            x->f_extra_outlet = NULL;
            return 0;
        }
        else if(!faust_io_manager_has_extra_output(x) && extraout)
        {
            x->f_extra_outlet = outlet_new((t_object *)x->f_owner, NULL);
            if(x->f_extra_outlet)
            {
                return 0;
            }
            pd_error(x->f_owner, "faustgen~: memory allocation failed - extra output");
            return 1;
        }
        return 0;
    }
    
    if(faust_io_manager_has_extra_output(x))
    {
        outlet_free(x->f_extra_outlet);
        x->f_extra_outlet = NULL;
    }
    for(i = couts; i > rnouts; --i)
    {
        outlet_free(x->f_outlets[i-1]);
        x->f_outlets[i-1] = NULL;
    }
    noutlets = (t_outlet **)resizebytes(x->f_outlets, sizeof(t_outlet*) * couts, sizeof(t_outlet*) * rnouts);
    if(noutlets)
    {
        for(i = couts; i < rnouts; ++i)
        {
            noutlets[i] = outlet_new((t_object *)x->f_owner, gensym("signal"));
            if(!noutlets[i])
            {
                pd_error(x->f_owner, "faustgen~: memory allocation failed - output %i", (int)i);
            }
        }
        x->f_outlets = noutlets;
        x->f_noutlets = rnouts;
        if(extraout)
        {
            x->f_extra_outlet = outlet_new((t_object *)x->f_owner, NULL);
            if(x->f_extra_outlet)
            {
                 return 0;
            }
            pd_error(x->f_owner, "faustgen~: memory allocation failed - extra output");
            return 1;
        }
        return 0;
    }
    pd_error(x->f_owner, "faustgen~: memory allocation failed - output");
    return 1;
}

//////////////////////////////////////////////////////////////////////////////////////////////////
//                                      PUBLIC INTERFACE                                        //
//////////////////////////////////////////////////////////////////////////////////////////////////

t_faust_io_manager* faust_io_manager_new(t_object* owner, t_canvas* canvas)
{
    t_faust_io_manager* x = (t_faust_io_manager*)getbytes(sizeof(t_faust_io_manager));
    if(x)
    {
        x->f_owner          = owner;
        x->f_canvas         = canvas;
        x->f_nsignals       = 0;
        x->f_signals        = NULL;
        x->f_ninlets        = 1;
        x->f_inlets         = getbytes(sizeof(t_inlet *));
        x->f_noutlets       = 0;
        x->f_outlets        = NULL;
        x->f_extra_outlet   = NULL;
        x->f_valid          = 0;
    }
    return x;
}

void faust_io_manager_free(t_faust_io_manager* x)
{
    faust_io_manager_free_inputs(x);
    faust_io_manager_free_outputs(x);
    faust_io_manager_free_signals(x);
    freebytes(x, sizeof(t_faust_io_manager));
}

size_t faust_io_manager_get_ninputs(t_faust_io_manager *x)
{
    return x->f_ninlets;
}

size_t faust_io_manager_get_noutputs(t_faust_io_manager *x)
{
    return x->f_noutlets;
}

char faust_io_manager_has_extra_output(t_faust_io_manager *x)
{
    return x->f_extra_outlet != NULL;
}

t_outlet* faust_io_manager_get_extra_output(t_faust_io_manager *x)
{
    return x->f_extra_outlet;
}

char faust_io_manager_init(t_faust_io_manager *x, int const nins, int const nouts, char const extraout)
{
    char valid = 0;
    char const redraw = x->f_owner->te_binbuf && gobj_shouldvis((t_gobj *)x->f_owner, x->f_canvas) && glist_isvisible(x->f_canvas);
    if(redraw)
    {
        gobj_vis((t_gobj *)x->f_owner, x->f_canvas, 0);
    }
    size_t const rnins = nins > 1 ? nins : 1;
    valid += faust_io_manager_resize_inputs(x, (size_t)rnins);
    valid += faust_io_manager_resize_outputs(x, (size_t)nouts, extraout);
    valid += faust_io_manager_resize_signals(x, (size_t)rnins + (size_t)nouts);
    if(redraw)
    {
        gobj_vis((t_gobj *)x->f_owner, x->f_canvas, 1);
        canvas_fixlinesfor(x->f_canvas, (t_text *)x->f_owner);
    }
    x->f_valid = (valid == 0);
    return valid;
}

#include <m_imp.h>

static char faust_io_manager_is_valid(t_faust_io_manager *x)
{
    if(!x->f_signals || !x->f_valid)
    {
        pd_error(x->f_owner, "faustgen~: something wrong happened during iolets allocation");
        return 0;
    }
    if(obj_nsiginlets(x->f_owner) != x->f_ninlets)
    {
        pd_error(x->f_owner, "faustgen~: number of signal inlets %i incompatible with internal %i", (int)x->f_ninlets, (int)obj_nsiginlets(x->f_owner));
        return 0;
    }
    if(obj_nsiginlets(x->f_owner) != x->f_ninlets)
    {
        pd_error(x->f_owner, "faustgen~: number of signal inlets %i incompatible with internal %i", (int)x->f_ninlets, (int)obj_nsiginlets(x->f_owner));
        return 0;
    }
    if(obj_nsigoutlets(x->f_owner) != x->f_noutlets)
    {
        pd_error(x->f_owner, "faustgen~: number of signal outlets %i incompatible with internal %i", (int)x->f_noutlets, (int)obj_nsigoutlets(x->f_owner));
        return 0;
    }
    if(x->f_ninlets + x->f_noutlets != x->f_nsignals)
    {
        pd_error(x->f_owner, "faustgen~: number of signals %i incompatible with number of iolets %i", (int)x->f_nsignals, (int)(x->f_ninlets + x->f_noutlets));
        return 0;
    }
    return 1;
}

char faust_io_manager_prepare(t_faust_io_manager *x, t_signal **sp)
{
    size_t i;
    if(!faust_io_manager_is_valid)
    {
        return 1;
    }
    for(i = 0; i < x->f_nsignals; ++i)
    {
        x->f_signals[i] = sp[i]->s_vec;
        if(x->f_signals[i] == NULL)
        {
            pd_error(x->f_owner, "faustgen~: the signal vector %i is empty", (int)i);
            return 1;
        }
    }
    return 0;
}

t_sample** faust_io_manager_get_input_signals(t_faust_io_manager *x)
{
    return x->f_signals;
}

t_sample** faust_io_manager_get_output_signals(t_faust_io_manager *x)
{
    return x->f_signals+x->f_ninlets;
}

void faust_io_manager_print(t_faust_io_manager* x, char const log)
{
    logpost(x->f_owner, 2+log, "             number of inputs: %i", (int)faust_io_manager_get_ninputs(x));
    logpost(x->f_owner, 2+log, "             number of outputs: %i", (int)faust_io_manager_get_noutputs(x));
    logpost(x->f_owner, 2+log, "             extra output: %s", faust_io_manager_has_extra_output(x) ? "true" : "false");
}
