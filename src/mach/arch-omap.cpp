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
                 // IRQs
                 "addlist irqs P2V(0xfffecb00) 0x40601c2 32 0\n"
                 "addlist irqs P2V(0xfffe0000) 0x0 32 0\n"
                 "addlist irqs P2V(0xfffe0100) 0 32 0\n"
                 "addlist irqs P2V(0xfffbc014) 0 32 0\n"
                 "addlist irqs P2V(0xfffbc814) 0 32 0\n"
                 "addlist irqs P2V(0xfffbd014) 0 32 0\n"
                 "addlist irqs P2V(0xfffbd814) 0 32 0\n"
                 "addlist irqs P2V(0xfffbe014) 0 32 0\n"
                 "addlist irqs P2V(0xfffbe814) 0 32 0\n"
                 // GPIOs
                 "addlist gpios P2V(0xfffbc000)\n"
                 "addlist gpios P2V(0xfffbc800)\n"
                 "addlist gpios P2V(0xfffbd000)\n"
                 "addlist gpios P2V(0xfffbd800)\n"
                 "addlist gpios P2V(0xfffbe000)\n"
                 "addlist gpios P2V(0xfffbe800)\n");
}
