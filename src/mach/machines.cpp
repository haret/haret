#include <windows.h>
#include <mmsystem.h>

#include "util.h" // ARRAY_SIZE
#include "memory.h" // mem_autodetect
#include "lateload.h" // setup_LateLoading
#include "output.h" // Output
#include "script.h" // setupCommands
#include "machines.h"

// Global current machine setting.
class Machine *Mach;

Machine::Machine()
    : name(""), PlatformType(L"PocketPC")
    , machType(0), fbDuringBoot(1)
{
    memset(OEMInfo, 0, sizeof(OEMInfo));
}

void
Machine::init()
{
}

int
Machine::preHardwareShutdown()
{
    return 0;
}

void
Machine::hardwareShutdown()
{
}

int
Machine::getBoardID()
{
    return -1;
}

const char *
Machine::getIrqName(uint)
{
    return "Unknown";
}

// Some defaults.
static Machine RefMachine;
static MachinePXA RefMachinePXA;
static MachinePXA27x RefMachinePXA27x;

// Symbols added by linker.
extern "C" {
    extern Machine *mach_start[];
    extern Machine *mach_end;
}

static Machine *
findMachineType()
{
    // Search for an exact match by SysParam stuff
    wchar_t oeminfo[128], platform[128];
    SystemParametersInfo(SPI_GETOEMINFO, sizeof(oeminfo), oeminfo, 0);
    SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(platform), platform, 0);
    Machine **p = mach_start;
    while (p < &mach_end) {
        Machine *m = *p;
        p++;
        Output("Looking at machine %s", m->name);
        if (wcscmp(platform, m->PlatformType) != 0)
            continue;
        for (uint32 j=0; j<ARRAY_SIZE(m->OEMInfo) && m->OEMInfo[j]; j++) {
            int len = wcslen(m->OEMInfo[j]);
            if (wcsncmp(oeminfo, m->OEMInfo[j], len) == 0)
                // Match
                return m;
        }
    }

    // Couldn't find a machine - try by architecture.
    if (RefMachinePXA27x.detect())
        return &RefMachinePXA27x;
    if (RefMachinePXA.detect())
        return &RefMachinePXA;
    return &RefMachine;
}

// Global detection mechanism
void
setupMachineType()
{
    if (Mach) {
        Output("Error: machine already defined to '%s'", Mach->name);
        return;
    }

    // Bind to DLLs dynamically.
    setup_LateLoading();

    // Detect the memory on the machine via wince first.
    Output("Detecting memory");
    mem_autodetect();

    // Determine what the current machine type is.
    Output("Detecting current machine");
    Mach = findMachineType();
    Output("Initializing for machine '%s'", Mach->name);
    Mach->init();
}
