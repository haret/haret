#include "arch-sa.h"
#include "script.h" // runMemScript

MachineSA::MachineSA()
{
    name = "Generic Intel StrongArm";
}

void
MachineSA::init()
{
    runMemScript(
        "set ramaddr 0xc0000000\n"
        );
}

int
MachineSA::detect()
{
    // TODO - need to implement detection system.
    return 0;
}
