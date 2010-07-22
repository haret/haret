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
#include "memory.h" // memPhysMap
#include "memcmds.h" // memPhysFill

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
    wchar_t wdllname[MAX_CMDLEN], wfuncname[MAX_CMDLEN];
    if (get_wtoken(&args, wdllname, ARRAY_SIZE(wdllname))
        || get_wtoken(&args, wfuncname, ARRAY_SIZE(wfuncname))) {
        ScriptError("Expected <dll name> <func name>");
        return;
    }
    tryLoadFunc(wdllname, wfuncname);
}
REG_CMD(0, "LOADFUNC", cmdLoadFunc,
        "LOADFUNC <dll name> <func name>\n"
        "  Return the address of the specified function in the given dll")

static void
cmdPatchFunc(const char *cmd, const char *args)
{
    wchar_t wdllname[MAX_CMDLEN], wfuncname[MAX_CMDLEN];
    uint32 returnValue;
    if (get_wtoken(&args, wdllname, ARRAY_SIZE(wdllname))
        || get_wtoken(&args, wfuncname, ARRAY_SIZE(wfuncname))) {
        ScriptError("Expected <dll name> <func name> [return value]");
        return;
    }
    uint32 vFuncAddr = (uint32)tryLoadFunc(wdllname, wfuncname);
    uint32 pFuncAddr = (uint32)memVirtToPhys(vFuncAddr);
    if (get_expression(&args, &returnValue))
    {
        if (returnValue > 0xFFF)
        {
            ScriptError("Cannot use return value larger than 0xFFF");
            return;
        }
        // return x
        memPhysFill(pFuncAddr, 1, 0xE12FFF1E, MO_SIZE32);
        memPhysFill(pFuncAddr + 4, 1, 0xE12FFF1E, MO_SIZE32);
        memPhysFill(pFuncAddr, 1, 0xE3A00000 | returnValue, MO_SIZE32);
        Output("Function '%ls' at phys address 0x%x patched to return %d", wfuncname, vFuncAddr, returnValue);
    }
    else
    {
        // void
        memPhysFill(pFuncAddr, 1, 0xE12FFF1E, MO_SIZE32);
        Output("Function '%ls' at phys address 0x%x patched to return void", wfuncname, vFuncAddr);
    }
}
REG_CMD(0, "PATCHFUNC", cmdPatchFunc,
        "PATCHFUNC <dll name> <func name> [return value]\n"
        "  Patches the given function to return void, or a given value")

