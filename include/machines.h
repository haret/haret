#ifndef _MACHINES_H
#define _MACHINES_H

#include "xtypes.h" // uint32

/*
 * Support for multiple machines types from within HaRET.
 *
 * To utilize this support, derive a new class from Machine and
 * register it using the REG_MACHINE(xxx) macro.  Set the OEMInfo
 * parameter to match the OEMInfo WinCE returns to declare a specific
 * machine.  Alternatively, one may leave OEMInfo empty and declare a
 * ->detect() method to declare an architecture.
 *
 * At startup, haret will test for all machines using the OEMInfo
 * data.  If no machines match, it will scan for architectures using
 * the detect() method.
 */

// Global current machine setting.
extern class Machine *Mach;

// Global detection mechanism
void setupMachineType();

// Typedefs
typedef void (__stdcall *startfunc_t)(char *kernel, uint32 mach, char *tags);

// Machine class base definition.
class Machine {
public:
    Machine();
    virtual ~Machine() { }

    const wchar_t *CPUInfo[16];
    const char *name, *archname;
    const wchar_t *OEMInfo[16];
    const wchar_t *PlatformType;
    int machType;
    int arm6mmu;

    virtual void init();
    virtual int preHardwareShutdown(struct fbinfo *);
    virtual void hardwareShutdown(struct fbinfo *);
    virtual int detect();
    void (*flushCache)(void);
    startfunc_t customStartFunc;
};

// Register a machine class to be scanned during haret start.
#define REGMACHINE(Mach)                                                \
static Mach Ref ##Mach;                                                 \
Machine *initVar ##Mach __attribute__ ((__section__ (".rdata.mach")))   \
    = &Ref ##Mach;

#endif // machines.h
