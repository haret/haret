/* Linux boot loading stub around haret.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 * Copyright (C) 2003 Andrew Zabolotny
 *
 * For conditions of use see file COPYING
 */

#include "output.h" // setupHaret
#include "linboot.h" // bootRamLinux
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
    shutdownHaret();

    return 0;
}
