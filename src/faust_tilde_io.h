/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#ifndef FAUST_TILDE_IO_H
#define FAUST_TILDE_IO_H

#include <m_pd.h>

struct _faust_io_manager;
typedef struct _faust_io_manager t_faust_io_manager;

t_faust_io_manager* faust_io_manager_new(t_object* owner, t_canvas* canvas);

void faust_io_manager_free(t_faust_io_manager* x);

size_t faust_io_manager_get_ninputs(t_faust_io_manager *x);

size_t faust_io_manager_get_noutputs(t_faust_io_manager *x);

char faust_io_manager_has_extra_output(t_faust_io_manager *x);

t_outlet* faust_io_manager_get_extra_output(t_faust_io_manager *x);

char faust_io_manager_init(t_faust_io_manager *x, int const nins, int const nouts, char const extraout);

void faust_io_manager_prepare(t_faust_io_manager *x, t_signal **sp);

t_sample** faust_io_manager_get_input_signals(t_faust_io_manager *x);

t_sample** faust_io_manager_get_output_signals(t_faust_io_manager *x);

char faust_io_manager_is_valid(t_faust_io_manager *x);

void faust_io_manager_print(t_faust_io_manager* x, char const log);

#endif
