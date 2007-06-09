#include "script.h" // runMemScript
#include "arch-omap.h"

MachineOMAP850::MachineOMAP850()
{
    name = "Generic TI OMAP";
    archname = "OMAP850";
}

int
MachineOMAP850::detect()
{
    // TODO - need to implement detection system.
    return 0;
}

void
MachineOMAP850::init()
{
    runMemScript("set ramaddr 0x10000000\n"
                 // GPIOs
                 "addlist gpios P2V(0xfffbc000)\n"
                 "addlist gpios P2V(0xfffbc800)\n"
                 "addlist gpios P2V(0xfffbd000)\n"
                 "addlist gpios P2V(0xfffbd800)\n");
}
