/* Bind to functions in DLLs at startup.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include "windows.h" // GetProcAddress

#include "output.h" // Output
#include "script.h" // REG_CMD
#include "lateload.h" // setup_LateLoading

// Symbols added by linker.
extern "C" {
    extern late_load_s latelist_start[];
    extern late_load_s latelist_end;
}
#define latelist_count (&latelist_end - latelist_start)

// Attempt to load in a function from a DLL.
static void *
tryLoadFunc(const wchar_t *dll, const wchar_t *funcname)
{
    HINSTANCE hi = LoadLibrary(dll);
    if (!hi) {
        Output("Unable to load library '%ls'", dll);
        return NULL;
    }

    void *func = (void*)GetProcAddress(hi, funcname);
    if (!func) {
        Output("Unable to find function '%ls' in library '%ls'"
               , funcname, dll);
        return NULL;
    }

    Output("Function '%ls' in library '%ls' at %p"
           , funcname, dll, func);
    return func;
}

// Attempt to load in the DLLs and bind all requested functions.
void
setup_LateLoading()
{
    Output("Loading dynamically bound functions");
    for (int i=0; i<latelist_count; i++) {
        struct late_load_s *ll = &latelist_start[i];

        void *func = tryLoadFunc(ll->dll, ll->funcname);
        if (! func)
            func = ll->alt;
        *(ll->funcptr) = func;
    }
}

static void
cmdLoadFunc(const char *cmd, const char *args)
{
    char dllname[MAX_CMDLEN], funcname[MAX_CMDLEN];
    if (get_token(&args, dllname, sizeof(dllname))
        || get_token(&args, funcname, sizeof(funcname))) {
        ScriptError("Expected <dll name> <func name>");
        return;
    }

    wchar_t wdllname[MAX_CMDLEN], wfuncname[MAX_CMDLEN];
    mbstowcs(wdllname, dllname, ARRAY_SIZE(wdllname));
    mbstowcs(wfuncname, funcname, ARRAY_SIZE(wfuncname));

    tryLoadFunc(wdllname, wfuncname);
}
REG_CMD(0, "LOADFUNC", cmdLoadFunc,
        "LOADFUNC <dll name> <func name>\n"
        "  Return the address of the specified function in the given dll")
