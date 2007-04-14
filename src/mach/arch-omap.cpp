#include "arch-omap.h"

MachineOMAP::MachineOMAP()
{
    name = "Generic TI OMAP";
    archname = "OMAP";
}

int
MachineOMAP::detect()
{
    // TODO - need to implement detection system.
    return 0;
}
