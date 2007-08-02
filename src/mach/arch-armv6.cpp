#include "cpu.h" // DEF_GETCPR
#include "arch-armv6.h"

// Assembler functions
extern "C" {
    void cpuFlushCache_arm6();
}

MachineArmV6::MachineArmV6()
{
    name = "Generic ARM v6";
    flushCache = cpuFlushCache_arm6;
}

DEF_GETCPR(get_p15r0, 15, 0, c0, c0, 0)

int
MachineArmV6::detect()
{
    uint32 p15r0 = get_p15r0();
    return ((p15r0 >> 24) == 'A'
            && ((p15r0 >> 16) & 0xf) == 7);
}

REGMACHINE(MachineArmV6)
