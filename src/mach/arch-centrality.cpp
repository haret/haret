// Support for "centrality" cpus.
#include "script.h" // runMemScript
#include "arch-arm.h" // cpuFlushCache_arm926
#include "arch-centrality.h"

MachineAT64x::MachineAT64x()
{
    name = "Generic Atlas";
    flushCache = cpuFlushCache_arm926;
    archname = "AT64x";
    CPUInfo[0] = L"AT64";
}

void
MachineAT64x::init()
{
    runMemScript(
        "set ramaddr 0xc0000000\n"
        );
}

REGMACHINE(MachineAT64x)
