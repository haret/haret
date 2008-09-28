// Support for Qualcomm MSMxxxx cpus.
#include "script.h" // runMemScript
#include "arch-arm.h" // cpuFlushCache_arm6
#include "arch-msm.h"

static void
defineMsmGpios()
{
    runMemScript(
        // out registers?
        "addlist gpios p2v(0xa9200800)\n"
        "addlist gpios p2v(0xa9300c00)\n"
        "addlist gpios p2v(0xa9200804)\n"
        "addlist gpios p2v(0xa9200808)\n"
        "addlist gpios p2v(0xa920080c)\n"
        // in registers?
        "addlist gpios p2v(0xa9200834)\n"
        "addlist gpios p2v(0xa9300c20)\n"
        "addlist gpios p2v(0xa9200838)\n"
        "addlist gpios p2v(0xa920083c)\n"
        "addlist gpios p2v(0xa9200840)\n"
        // out enable registers?
        "addlist gpios p2v(0xa9200810)\n"
        "addlist gpios p2v(0xa9300c08)\n"
        "addlist gpios p2v(0xa9200814)\n"
        "addlist gpios p2v(0xa9200818)\n"
        "addlist gpios p2v(0xa920081c)\n"
        );
}


/****************************************************************
 * MSM 7xxxA
 ****************************************************************/

MachineMSM7xxxA::MachineMSM7xxxA()
{
    name = "Generic MSM7xxxA";
    flushCache = cpuFlushCache_arm6;
    arm6mmu = 1;
    archname = "MSM7xxxA";
    CPUInfo[0] = L"MSM7201A";
}

void
MachineMSM7xxxA::init()
{
    runMemScript(
        "set ramaddr 0x10000000\n"
        "addlist irqs p2v(0xc0000080) 0x100 32 0\n"
        "addlist irqs p2v(0xc0000084) 0 32 0\n"
        );
    defineMsmGpios();
}

REGMACHINE(MachineMSM7xxxA)


/****************************************************************
 * MSM 7xxx
 ****************************************************************/

MachineMSM7xxx::MachineMSM7xxx()
{
    name = "Generic MSM7xxx";
    flushCache = cpuFlushCache_arm6;
    arm6mmu = 1;
    archname = "MSM7xxx";
    CPUInfo[0] = L"MSM7500";
    CPUInfo[1] = L"MSM7200";
}

void
MachineMSM7xxx::init()
{
    runMemScript(
        "set ramaddr 0x10000000\n"
        "addlist irqs p2v(0xc0000080) 0x100 32 0\n"
        "addlist irqs p2v(0xc0000084) 0 32 0\n"
        );
    defineMsmGpios();
}

REGMACHINE(MachineMSM7xxx)
