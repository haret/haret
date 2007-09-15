// Support for Qualcomm MSMxxxx cpus.
#include "script.h" // runMemScript
#include "arch-msm.h"

MachineMSM7500::MachineMSM7500()
{
    name = "Generic MSM7500";
    archname = "MSM7500";
    CPUInfo[0] = L"MSM7500";
}

void
MachineMSM7500::init()
{
    runMemScript(
        "set ramaddr 0x10000000\n"
        );
}
