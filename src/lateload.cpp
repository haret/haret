/* Bind to functions in DLLs at startup.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include "windows.h" //

#include "output.h" // Output
#include "lateload.h"

// Symbols added by linker.
extern "C" {
    extern late_load_s latelist_start[];
    extern late_load_s latelist_end;
}
#define latelist_count (&latelist_end - latelist_start)

// Attempt to load in the DLLs and bind all requested functions.
void
setup_LateLoading()
{
    for (int i=0; i<latelist_count; i++) {
        struct late_load_s *ll = &latelist_start[i];

        Output("Trying to load library '%ls'", ll->dll);
        HINSTANCE hi = LoadLibrary(ll->dll);
        if (!hi) {
            Output("Unable to load library '%ls'", ll->dll);
            continue;
        }

        void *func = (void*)GetProcAddress(hi, ll->funcname);
        if (!func) {
            Output("Unable to find function '%ls' in library '%ls'"
                   , ll->funcname, ll->dll);
            continue;
        }
        *(ll->funcptr) = func;
        Output("Function '%ls' in library '%ls' at %p"
               , ll->funcname, ll->dll, func);
    }
}
