#include "arch-sa.h"
#include "script.h" // runMemScript

MachineSA::MachineSA()
{
    name = "Generic Intel StrongArm";
    archname = "SA1110";
    CPUInfo[0] = L"SA1110";
    CPUInfo[1] = L"SA110";
}

void
MachineSA::init()
{
    runMemScript(
        "set ramaddr 0xc0000000\n"

        // Interrupt controller irq pending (ICIP)
        "addlist IRQS p2v(0x90050000) 0 32 0\n"

        // GPIO edge detect status (GEDR)
        "addlist IRQS p2v(0x90040018) 0 32 0\n"

        // GPIO pin level (GPLR)
        "addlist GPIOS p2v(0x90040000)\n"

        // GPIO pin direction (GPDR)
        "addlist GPIOS p2v(0x90040004)\n"

        // GPIO output set (GPSR)
        "addlist GPIOS p2v(0x90040008)\n"

        // GPIO output clear (GPCR)
        "addlist GPIOS p2v(0x9004000C)\n"

        // GPIO rising edge (GRER)
        "addlist GPIOS p2v(0x90040010)\n"

        // GPIO falling edge (GFER)
        "addlist GPIOS p2v(0x90040014)\n"

        // GPIO alt function (GAFR)
        "addlist GPIOS p2v(0x9004001C)\n"

        );
}

int
MachineSA::detect()
{
    // TODO - need to implement detection system.
    return 0;
}

REGMACHINE(MachineSA)
