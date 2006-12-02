#include "arch-pxa.h" // MachinePXA
#include "mach-types.h"
#include "memory.h" // memPhysSize
//#include "asic3.h"

class MachBlueangel : public MachinePXA {
public:
    MachBlueangel() {
        name = "Blueangel";
        OEMInfo[0] = L"PH20";
        machType = MACH_TYPE_BLUEANGEL;
    }
    void init() {
#if 0
        asic3_gpio_base=0x0c000000;
        asic3_sdio_base=0x0e000000;
        asic3_bus_shift=1;
#endif
        memPhysSize=128*1024*1024;
    }
};

REGMACHINE(MachBlueangel)
