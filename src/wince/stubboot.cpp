/* Linux boot loading stub around haret.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 * Copyright (C) 2003 Andrew Zabolotny
 *
 * For conditions of use see file COPYING
 */

#include "output.h" // setupOutput
#include "machines.h" // setupMachineType
#include "linboot.h" // bootRamLinux
#include "memory.h" // memPhysReset
#include "script.h" // REG_CMD
#include "haret.h"

// Symbols surrounding kernel code added by kernelfiles.S
extern "C" {
    extern char kernel_data[];
    extern char kernel_data_end;
    extern char initrd_data[];
    extern char initrd_data_end;
    extern char script_data[];
}

// Boot kernel linked into exe.
static void
ramboot(const char *cmd, const char *args)
{
    uint32 kernelSize = &kernel_data_end - kernel_data;
    uint32 initrdSize = &initrd_data_end - initrd_data;
    bootRamLinux(kernel_data, kernelSize, initrd_data, initrdSize);
}
REG_CMD(0, "RAMBOOT|LINUX", ramboot,
        "RAMBOOTLINUX\n"
        "  Start booting linux kernel. See HELP VARS for variables affecting boot.")

// Run a haret script that is loaded into memory.
static void
runMemScript(const char *script)
{
    const char *s = script;
    for (int line = 1; *s; line++) {
        const char *lineend = strchr(s, '\n');
        const char *nexts;
        if (! lineend) {
            lineend = s + strlen(s);
            nexts = lineend;
        } else {
            nexts = lineend + 1;
        }
        if (lineend > s && lineend[-1] == '\r')
            lineend--;
        uint len = lineend - s;
        char str[MAX_CMDLEN];
        if (len >= sizeof(str))
            len = sizeof(str) - 1;
        memcpy(str, s, len);
        str[len] = 0;
        scrInterpret(str, line);
        s = nexts;
    }
}

HINSTANCE hInst;
HWND MainWindow = 0;

int
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;

    // Setup haret.
    setupHaret();

    // Run linked in script.
    runMemScript(script_data);

    // Shutdown.
    Output("Shutting down");
    memPhysReset();

    closeLogFile();

    return 0;
}
