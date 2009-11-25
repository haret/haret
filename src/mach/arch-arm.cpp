#include "cpu.h" // DEF_GETCPR
#include "arch-arm.h"

DEF_GETCPR(get_p15r0, 15, 0, c0, c0, 0)


/****************************************************************
 * ARM 920t
 ****************************************************************/

Machine920t::Machine920t()
{
    name = "Generic ARM 920t";
    flushCache = cpuFlushCache_arm920;
}

int
Machine920t::detect()
{
    uint32 p15r0 = get_p15r0();
    return ((p15r0 >> 24) == 'A'
            && ((p15r0 >> 20) & 0xf) == 1
            && ((p15r0 >> 16) & 0xf) == 2
            && ((p15r0 >> 4) & 0xfff) == 0x920);
}

REGMACHINE(Machine920t)


/****************************************************************
 * ARM 926
 ****************************************************************/

Machine926::Machine926()
{
    name = "Generic ARM 926";
    flushCache = cpuFlushCache_arm926;
}

int
Machine926::detect()
{
    uint32 p15r0 = get_p15r0();
    return ((p15r0 >> 24) == 'A'
            && ((p15r0 >> 20) & 0xf) == 0
            && ((p15r0 >> 16) & 0xf) == 6
            && ((p15r0 >> 4) & 0xfff) == 0x926);
}

REGMACHINE(Machine926)


/****************************************************************
 * ARM v6
 ****************************************************************/

MachineArmV6::MachineArmV6()
{
    name = "Generic ARM v6";
    flushCache = cpuFlushCache_arm6;
    arm6mmu = 1;
}

int
MachineArmV6::detect()
{
    uint32 p15r0 = get_p15r0();
    return ((p15r0 >> 24) == 'A'
            && ((p15r0 >> 16) & 0xf) == 7);
}

REGMACHINE(MachineArmV6)

/****************************************************************
 * ARM v7
 ****************************************************************/

MachineArmV7::MachineArmV7()
{
    name = "Generic ARM v7";
    flushCache = cpuFlushCache_arm7;
    arm6mmu = 1;
}

int
MachineArmV7::detect()
{
    uint32 p15r0 = get_p15r0();
    return (((p15r0 >> 24) == 'A' || (p15r0 >> 24) == 'Q') // Q = Qualcomm, see cpu.cpp
            && ((p15r0 >> 16) & 0xf) == 15);
}

REGMACHINE(MachineArmV7)
