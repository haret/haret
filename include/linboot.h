/*
    ARM Linux Boot routines
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _LINBOOT_H
#define _LINBOOT_H

extern char *bootKernel;
extern char *bootInitrd;
extern char *bootCmdline;
extern uint32 bootSpeed;
extern uint32 bootMachineType;

extern void bootLinux ();

#endif /* _LINBOOT_H */
