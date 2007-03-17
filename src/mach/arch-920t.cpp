#include "cpu.h" // DEF_GETCPR
#include "arch-920t.h"

// Assembler functions
extern "C" {
    void cpuFlushCache_arm920();
}

Machine920t::Machine920t()
{
    name = "Generic ARM 920t";
    flushCache = cpuFlushCache_arm920;
}

DEF_GETCPR(get_p15r0, 15, 0, c0, c0, 0)

int
Machine920t::detect()
{
    uint32 p15r0 = get_p15r0();
    return ((p15r0 >> 24) == 'A'
            && ((p15r0 >> 20) & 0x7) == 1
            && ((p15r0 >> 16) & 0x7) == 2
            && ((p15r0 >> 4) & 0x7f) == 0x920);
}
