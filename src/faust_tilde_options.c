/*
// Copyright (c) 2018 - GRAME CNCM - CICM - ANR MUSICOLL - Pierre Guillot.
// For information on usage and redistribution, and for a DISCLAIMER OF ALL
// WARRANTIES, see the file, "LICENSE.txt," in this distribution.
*/


#include "faust_tilde_options.h"
#include <string.h>

#define MAXFAUSTSTRING 4096

typedef struct _faust_opt_manager
{
    t_object*   f_owner;
    char*       f_default_include;
    size_t      f_noptions;
    char**      f_options;
    t_symbol*   f_directory;
    t_symbol*   f_temp_path;
    char        f_use_default_include;
}t_faust_opt_manager;


// LOCATE DEFAULT INCLUDE PATH
//////////////////////////////////////////////////////////////////////////////////////////////////
static void faust_opt_manager_get_default_include_path(t_faust_opt_manager *x)
{
    char const* path = class_gethelpdir(pd_class((t_pd *)x->f_owner));
    if(path)
    {
        x->f_default_include = (char *)getbytes((strnlen(path, MAXPDSTRING) + 7) * sizeof(char));
        if(x->f_default_include)
        {
            sprintf(x->f_default_include, "%s/libs/", path);
            return;
        }
        pd_error(x->f_owner, "faustgen~: memory allocation failed - include path");
        return;
    }
    pd_error(x->f_owner, "faustgen~: cannot locate the include path");
    return;
}

static void faust_opt_manager_free_default_include_path(t_faust_opt_manager *x)
{
    if(x->f_default_include)
    {
        freebytes(x->f_default_include, strnlen(x->f_default_include, MAXPDSTRING) * sizeof(char));
    }
    x->f_default_include = NULL;
}

// COMPILE OPTIONS
//////////////////////////////////////////////////////////////////////////////////////////////////

static void faust_opt_manager_free_compile_options(t_faust_opt_manager *x)
{
    if(x->f_options && x->f_noptions)
    {
        size_t i;
        size_t noptions = x->f_use_default_include ? x->f_noptions - 1 : x->f_noptions;
        for(i = 0; i < noptions; ++i)
        {
            if(x->f_options[i])
            {
                freebytes(x->f_options[i], strnlen(x->f_options[i], MAXFAUSTSTRING));
            }
            x->f_options[i] = NULL;
        }
        freebytes(x->f_options, x->f_noptions * sizeof(char *));
    }
    x->f_options    = NULL;
    x->f_noptions   = 0;
    x->f_use_default_include = 0;
}

char faust_opt_manager_parse_compile_options(t_faust_opt_manager *x, size_t const argc, const t_atom* argv)
{
    char has_include = 0;
    faust_opt_manager_free_compile_options(x);
    if(argc && argv)
    {
        x->f_options = getbytes(argc * sizeof(char *));
        if(x->f_options)
        {
            size_t i;
            for(i = 0; i < argc; ++i)
            {
                x->f_options[i] = (char *)getbytes(MAXFAUSTSTRING * sizeof(char));
                if(x->f_options[i])
                {
                    if(argv[i].a_type == A_FLOAT)
                    {
                        sprintf(x->f_options[i], "%i", (int)argv[i].a_w.w_float);
                    }
                    else if(argv[i].a_type == A_SYMBOL && argv[i].a_w.w_symbol)
                    {
                        sprintf(x->f_options[i], "%s", argv[i].a_w.w_symbol->s_name);
                        if(!strncmp(x->f_options[i], "-I", 2))
                        {
                            has_include = 1;
                        }
                    }
                    else
                    {
                        pd_error(x->f_owner, "faustgen~: option type invalid");
                        memset(x->f_options[i], 0, MAXFAUSTSTRING);
                    }
                    x->f_noptions = i+1;
                }
                else
                {
                    pd_error(x->f_owner, "faustgen~: memory allocation failed - compile option %i", (int)i);
                    x->f_noptions = i;
                    return -1;
                }
            }
        }
        else
        {
            pd_error(x->f_owner, "faustgen~: memory allocation failed - compile options vector");
            x->f_noptions = 0;
            return -1;
        }
    }
    if(!has_include)
    {
        char **temp = resizebytes(x->f_options, (argc * sizeof(char *)), ((argc + 2) * sizeof(char *)));
        if(temp)
        {
            x->f_options = temp;
            x->f_options[argc] = (char *)getbytes(3 * sizeof(char));
            if(x->f_options[argc])
            {
                sprintf(x->f_options[argc], "%s", "-I");
            }
            else
            {
                pd_error(x, "faustgen~: memory allocation failed - compile option");
                x->f_noptions = argc;
                return -1;
            }
            
            x->f_options[argc+1] = x->f_default_include;
            x->f_noptions = argc+2;
            x->f_use_default_include = 1;
            return 0;
        }
        else
        {
            pd_error(x->f_owner, "faustgen~: memory allocation failed - compile options for default include");
            return -1;
        }
    }
    return 0;
}


//////////////////////////////////////////////////////////////////////////////////////////////////
//                                      PUBLIC INTERFACE                                        //
//////////////////////////////////////////////////////////////////////////////////////////////////

t_faust_opt_manager* faust_opt_manager_new(t_object* owner, t_canvas* canvas)
{
    t_faust_opt_manager* x = (t_faust_opt_manager*)getbytes(sizeof(t_faust_opt_manager));
    if(x)
    {
        x->f_owner                  = owner;
        x->f_default_include        = NULL;
        x->f_options                = NULL;
        x->f_noptions               = 0;
        x->f_use_default_include    = 0;
        x->f_directory              = canvas_getdir(canvas);
        faust_opt_manager_get_default_include_path(x);
    }
    return x;
}

void faust_opt_manager_free(t_faust_opt_manager* x)
{
    faust_opt_manager_free_default_include_path(x);
    freebytes(x, sizeof(t_faust_opt_manager));
}

size_t faust_opt_manager_get_noptions(t_faust_opt_manager* x)
{
    return x->f_noptions;
}

char const** faust_opt_manager_get_options(t_faust_opt_manager* x)
{
    return (char const**)x->f_options;
}

char const* faust_opt_manager_get_full_path(t_faust_opt_manager* x, char const* name)
{
    if(sys_isabsolutepath(name))
    {
        return name;
    }
    else if(x->f_directory && x->f_directory->s_name && name)
    {
        char* file = (char *)getbytes(MAXFAUSTSTRING * sizeof(char));
        if(file)
        {
            sprintf(file, "%s/%s.dsp", x->f_directory->s_name, name);
            x->f_temp_path = gensym(file);
            freebytes(file, MAXFAUSTSTRING * sizeof(char));
            return x->f_temp_path->s_name;
        }
        pd_error(x->f_owner, "faustgen~: memory allocation failed - path");
        return NULL;
    }
    pd_error(x->f_owner, "faustgen~: invalid path or name");
    return NULL;
}

void faust_opt_manager_print(t_faust_opt_manager const *x, char const log)
{
    size_t i;
    for(i = 0; i < x->f_noptions; ++i)
    {
        logpost(x->f_owner, 2+log, "             option %i: %s", (int)i, x->f_options[i]);
    }
}
