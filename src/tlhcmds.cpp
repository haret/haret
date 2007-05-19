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
#include "cpu.h" // MVAddr

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
    char name[MAX_CMDLEN];
    if (get_token(&args, name, sizeof(name))) {
        ScriptError("process name expected");
        return;
    }
    wchar_t wname[MAX_CMDLEN];
    mbstowcs(wname, name, ARRAY_SIZE(wname));
    Output("Looking to kill '%ls'", wname);

    HANDLE hTH = late_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hTH == INVALID_HANDLE_VALUE) {
        Output("Unable to create tool help snapshot");
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);

    for (int ret=late_Process32First(hTH, &pe); ret
             ; ret=late_Process32Next(hTH, &pe))
        if (_wcsicmp(wname, pe.szExeFile) == 0) {
            HANDLE hproc = OpenProcess(0, 0, pe.th32ProcessID);
            Output("Found '%ls' with pid %08lx / handle %p"
                   , pe.szExeFile, pe.th32ProcessID, hproc);

            if (hproc != INVALID_HANDLE_VALUE && hproc != NULL) {
                TerminateProcess(hproc, 0);
                CloseHandle(hproc);
                break;
            }
        }

    late_CloseToolhelp32Snapshot(hTH);
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

    for (int ret=late_Process32First(hTH, &pe); ret
             ; ret=late_Process32Next(hTH, &pe))
        Output("ps: pid=%08lx ppid=%lx pmem=%08lx"
               " tcnt=%03ld perm=%08lx procname=%ls",
               pe.th32ProcessID, pe.th32ParentProcessID, pe.th32MemoryBase
               , pe.cntThreads, pe.th32AccessKey, pe.szExeFile);

    late_CloseToolhelp32Snapshot(hTH);
}
REG_CMD(tlhAvail, "PS", psDump,
        "PS\n"
        "  List wince process information.")

static void
modDump(const char *cmd, const char *args)
{
    uint32 pid = 0;
    HANDLE hTH;
    if (get_expression(&args, &pid))
        hTH = late_CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    else
        hTH = late_CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE|TH32CS_GETALLMODS, 0);
    if (hTH == INVALID_HANDLE_VALUE) {
        Output("Unable to create tool help snapshot");
        return;
    }

    MODULEENTRY32 me;
    me.dwSize = sizeof(me);

    for (int ret=late_Module32First(hTH, &me); ret
             ; ret=late_Module32Next(hTH, &me))
        Output("%4ld fl=%08lx mid=%08lx pid=%08lx gusg=%3ld pusg=%08lx"
               " base=%p size=%08lx hmod=%p mod=%ls exe=%ls",
               me.dwSize, me.dwFlags, me.th32ModuleID, me.th32ProcessID,
               me.GlblcntUsage, me.ProccntUsage,
               me.modBaseAddr, me.modBaseSize,
               me.hModule, me.szModule, me.szExePath);

    late_CloseToolhelp32Snapshot(hTH);
}
REG_CMD(tlhAvail, "LSMOD", modDump,
        "LSMOD <pid>\n"
        "  List wince modules.")

static void
cmd_addr2module(const char *cmd, const char *args)
{
    uint32 addr;
    if (!get_expression(&args, &addr)) {
        ScriptError("virtual address expected");
        return;
    }
    addr = MVAddr(addr);

    HANDLE hTH = late_CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (hTH == INVALID_HANDLE_VALUE) {
        Output("Unable to create tool help snapshot");
        return;
    }

    PROCESSENTRY32 pe;
    pe.dwSize = sizeof(pe);
    uint32 pid = 0;
    uint32 membase = 0;

    for (int ret=late_Process32First(hTH, &pe); ret
             ; ret=late_Process32Next(hTH, &pe)) {
        membase = pe.th32MemoryBase;
        if (membase <= addr && membase + 0x02000000 > addr) {
            Output("Address %08x in process: %ls (%08x - %08x)"
                   , addr, pe.szExeFile, membase, membase + 0x02000000);
            pid = pe.th32ProcessID;
            break;
        }
    }

    late_CloseToolhelp32Snapshot(hTH);

    if (!pid) {
        Output("Address %08x not process specific", addr);
        hTH = late_CreateToolhelp32Snapshot(
            TH32CS_SNAPMODULE|TH32CS_GETALLMODS, 0);
    } else {
        hTH = late_CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, pid);
    }
    if (hTH == INVALID_HANDLE_VALUE) {
        Output("Unable to create tool help snapshot");
        return;
    }

    MODULEENTRY32 me;
    me.dwSize = sizeof(me);
    for (int ret=late_Module32First(hTH, &me); ret
             ; ret=late_Module32Next(hTH, &me)) {
        uint32 a = (uint32)me.modBaseAddr;
        if (pid && a < 0x02000000)
            a |= membase;
        if (a <= addr && a + me.modBaseSize > addr) {
            Output("  in module: %ls (%08x - %08x)", me.szModule
                   , a, (uint32)(a + me.modBaseSize));
            break;
        }
    }

    late_CloseToolhelp32Snapshot(hTH);
}
REG_CMD(tlhAvail, "ADDR2MOD", cmd_addr2module,
        "ADDR2MOD <virtual address>\n"
        "  Lookup which process (and module) owns a given virtual address.")
