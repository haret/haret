#include "script.h" // runMemScript
#include "arch-omap.h"

MachineOMAP850::MachineOMAP850()
{
    name = "Generic TI OMAP";
    archname = "OMAP850";
    CPUInfo[0] = L"OMAP850";
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
                 // DATA_INPUT
                 "addlist gpios P2V(0xfffbc000)\n"
                 "addlist gpios P2V(0xfffbc800)\n"
                 "addlist gpios P2V(0xfffbd000)\n"
                 "addlist gpios P2V(0xfffbd800)\n"
                 "addlist gpios P2V(0xfffbe000)\n"
                 "addlist gpios P2V(0xfffbe800)\n"
                 // DATA_OUTPUT
                 "addlist gpios P2V(0xfffbc004)\n"
                 "addlist gpios P2V(0xfffbc804)\n"
                 "addlist gpios P2V(0xfffbd004)\n"
                 "addlist gpios P2V(0xfffbd804)\n"
                 "addlist gpios P2V(0xfffbe004)\n"
                 "addlist gpios P2V(0xfffbe804)\n"
                 // DIR_CONTROL
                 "addlist gpios P2V(0xfffbc008)\n"
                 "addlist gpios P2V(0xfffbc808)\n"
                 "addlist gpios P2V(0xfffbd008)\n"
                 "addlist gpios P2V(0xfffbd808)\n"
                 "addlist gpios P2V(0xfffbe008)\n"
                 "addlist gpios P2V(0xfffbe808)\n"
                 // INT_CONTROL
                 "addlist gpios P2V(0xfffbc00c)\n"
                 "addlist gpios P2V(0xfffbc80c)\n"
                 "addlist gpios P2V(0xfffbd00c)\n"
                 "addlist gpios P2V(0xfffbd80c)\n"
                 "addlist gpios P2V(0xfffbe00c)\n"
                 "addlist gpios P2V(0xfffbe80c)\n"
                 // INT_MASK
                 "addlist gpios P2V(0xfffbc010)\n"
                 "addlist gpios P2V(0xfffbc810)\n"
                 "addlist gpios P2V(0xfffbd010)\n"
                 "addlist gpios P2V(0xfffbd810)\n"
                 "addlist gpios P2V(0xfffbe010)\n"
                 "addlist gpios P2V(0xfffbe810)\n");
}

REGMACHINE(MachineOMAP850)
