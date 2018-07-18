/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#ifndef FAUST_TILDE_OPTIONS_H
#define FAUST_TILDE_OPTIONS_H

#include <m_pd.h>

struct _faust_opt_manager;
typedef struct _faust_opt_manager t_faust_opt_manager;

t_faust_opt_manager* faust_opt_manager_new(t_object* owner);

void faust_opt_manager_free(t_faust_opt_manager* x);

char faust_opt_manager_parse_compile_options(t_faust_opt_manager *x, size_t const argc, const t_atom* argv);

size_t faust_opt_manager_get_noptions(t_faust_opt_manager* x);

char const** faust_opt_manager_get_options(t_faust_opt_manager* x);

#endif
