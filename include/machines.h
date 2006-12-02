#ifndef _MACHINES_H
#define _MACHINES_H

#include "xtypes.h" // uint32

/*
 * Support for multiple machines types from within HaRET.
 */

// Global current machine setting.
extern class Machine *Mach;

// Global detection mechanism
void setupMachineType();

// Machine class base definition
class Machine {
public:
    Machine();
    virtual ~Machine() { }

    const char *name;
    const wchar_t *OEMInfo[16];
    const wchar_t *PlatformType;
    int machType;
    int fbDuringBoot;

    virtual void init();
    virtual int preHardwareShutdown();
    virtual void hardwareShutdown();
    virtual int getBoardID();
    virtual const char *getIrqName(uint);
};

// Register a machine class to be scanned during haret start.
#define REGMACHINE(Mach)                                                \
static Mach Ref ##Mach;                                                 \
Machine *initVar ##Mach __attribute__ ((__section__ (".rdata.mach")))   \
    = &Ref ##Mach;

#endif // machines.h
