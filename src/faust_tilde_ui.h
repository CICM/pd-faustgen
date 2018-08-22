/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/

#ifndef FAUST_TILDE_UI_H
#define FAUST_TILDE_UI_H

#include <m_pd.h>

struct _faust_ui_manager;
typedef struct _faust_ui_manager t_faust_ui_manager;

t_faust_ui_manager* faust_ui_manager_new(t_object* owner);

void faust_ui_manager_init(t_faust_ui_manager *x, void* dspinstance);

void faust_ui_manager_free(t_faust_ui_manager *x);

void faust_ui_manager_clear(t_faust_ui_manager *x);

char faust_ui_manager_set_value(t_faust_ui_manager *x, t_symbol const *name, t_float const f);

char faust_ui_manager_get_value(t_faust_ui_manager const *x, t_symbol const *name, t_float* f);

void faust_ui_manager_save_states(t_faust_ui_manager *x);

void faust_ui_manager_restore_states(t_faust_ui_manager *x);

void faust_ui_manager_print(t_faust_ui_manager const *x, char const log);

#endif
