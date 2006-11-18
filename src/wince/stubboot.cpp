/* Linux boot loading stub around haret.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 * Copyright (C) 2003 Andrew Zabolotny
 *
 * For conditions of use see file COPYING
 */

#include "output.h" // setupOutput
#include "util.h" // preparePath
#include "machines.h" // setupMachineType
#include "linboot.h" // bootRamLinux
#include "memory.h" // memPhysReset
#include "script.h" // to override get_token, etc.
#include "haret.h"

// Stub some functions in script.cpp.
bool get_expression(const char **s, uint32 *v, int priority, int flags)
{
    return 0;
}
char *get_token(const char **s)
{
    return NULL;
}
uint ScriptLine;

// Symbols surrounding kernel code added by linfiles.S
extern "C" {
    extern char kernel_data;
    extern char kernel_data_end;
    extern char initrd_data;
    extern char initrd_data_end;
    extern char cmdline_data;
}

HINSTANCE hInst;
HWND MainWindow = 0;

int
WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
        LPTSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;

    // Initialize the path so fnprepare() works.
    preparePath(hInstance);

    // Prep for early output.
    setupOutput();

    // Detect some system settings
    setupMachineType();

    // Override command line (of one set).
    char *cmd = &cmdline_data;
    if (!cmd[0])
        cmd = NULL;

    // Boot new kernel.
    uint32 kernelSize = &kernel_data_end - &kernel_data;
    uint32 initrdSize = &initrd_data_end - &initrd_data;
    bootRamLinux(&kernel_data, kernelSize, &initrd_data, initrdSize, cmd);

    Output("Shutting down");
    memPhysReset();

    closeLogFile();

    return 0;
}
