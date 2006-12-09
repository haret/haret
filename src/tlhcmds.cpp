/* Commands that use the toolhelp library.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 * Copyright (C) 2003 Andrew Zabolotny
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <windows.h>
#include <tlhelp32.h>

#include "xtypes.h" // uint
#include "lateload.h" // LATE_LOAD
#include "output.h" // Output
#include "script.h" // REG_CMD

LATE_LOAD(CreateToolhelp32Snapshot, "toolhelp")
LATE_LOAD(Process32First, "toolhelp")
LATE_LOAD(Process32Next, "toolhelp")
LATE_LOAD(Module32First, "toolhelp")
LATE_LOAD(Module32Next, "toolhelp")
LATE_LOAD(CloseToolhelp32Snapshot, "toolhelp")

static int
tlhAvail(void)
{
    return (late_CreateToolhelp32Snapshot
            && late_Process32First && late_Process32Next
            && late_Module32First && late_Module32Next
            && late_CloseToolhelp32Snapshot);
}

// Find a process by name and terminate it.
static void
cmd_kill(const char *cmd, const char *args)
{
    char *name = get_token(&args);
    if (!name) {
        Output(C_ERROR "line %d: process name expected", ScriptLine);
        return;
    }
    wchar_t wname[200];
    mbstowcs(wname, name, ARRAY_SIZE(wname));
    Output("Looking to kill '%ls'", wname);

    HANDLE hts = late_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hts == INVALID_HANDLE_VALUE) {
        Output("Unable to create tool help snapshot");
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(PROCESSENTRY32);

    if (late_Process32First(hts, &pe)) {
        do {
            if (wcsicmp(wname, pe.szExeFile) == 0) {
                HANDLE hproc = OpenProcess(0, 0, pe.th32ProcessID);
                Output("Found '%ls' with pid %08lx / handle %p"
                       , pe.szExeFile, pe.th32ProcessID, hproc);

                if (hproc != INVALID_HANDLE_VALUE && hproc != NULL) {
                    TerminateProcess(hproc, 0);
                    CloseHandle(hproc);
                    break;
                }
            }
        } while (late_Process32Next(hts, &pe));
    }

    late_CloseToolhelp32Snapshot(hts);
}
REG_CMD(tlhAvail, "KILL", cmd_kill,
        "KILL <process name>\n"
        "  Terminate the process with the specified name.")

static void
psDump(const char *cmd, const char *args)
{
    HANDLE hTH = late_CreateToolhelp32Snapshot(
        TH32CS_SNAPPROCESS|TH32CS_SNAPHEAPLIST, 0);
    if (hTH == INVALID_HANDLE_VALUE) {
        Output("Unable to create tool help snapshot");
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);

    if (late_Process32First(hTH, &pe)) {
        do {
            Output("ps: pid=%08lx ppid=%lx pmem=%08lx"
                   " tcnt=%03ld perm=%08lx procname=%ls",
                   pe.th32ProcessID, pe.th32ParentProcessID, pe.th32MemoryBase
                   , pe.cntThreads, pe.th32AccessKey, pe.szExeFile);
        } while (late_Process32Next(hTH, &pe));
    }

    late_CloseToolhelp32Snapshot(hTH);
}
REG_CMD(tlhAvail, "PS", psDump,
        "PS\n"
        "  List wince process information.")

static void
modDump(const char *cmd, const char *args)
{
    HANDLE hTH = late_CreateToolhelp32Snapshot(
        TH32CS_SNAPMODULE|TH32CS_GETALLMODS, 0);
    if (hTH == INVALID_HANDLE_VALUE) {
        Output("Unable to create tool help snapshot");
        return;
    }

    MODULEENTRY32 me;
    me.dwSize = sizeof(me);

    if (late_Module32First(hTH, &me)) {
        do {
            Output("%4ld fl=%08lx mid=%08lx pid=%08lx gusg=%3ld pusg=%08lx"
                   " base=%p size=%08lx hmod=%p mod=%ls exe=%ls",
                   me.dwSize, me.dwFlags, me.th32ModuleID, me.th32ProcessID,
                   me.GlblcntUsage, me.ProccntUsage,
                   me.modBaseAddr, me.modBaseSize,
                   me.hModule, me.szModule, me.szExePath);
        } while (late_Module32Next(hTH, &me));
    }

    late_CloseToolhelp32Snapshot(hTH);
}
REG_CMD(tlhAvail, "LSMOD", modDump,
        "LSMOD\n"
        "  List wince modules.")
