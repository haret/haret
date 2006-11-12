#include "machines.h"
#include "mach-types.h"
//#include "asic3.h"

class MachBlueangel : public MachinePXA {
public:
    MachBlueangel() {
        name = "Blueangel";
        OEMInfo[0] = L"PH20";
        machType = MACH_TYPE_BLUEANGEL;
    }
#if 0
    void init() {
        asic3_gpio_base=0x0c000000;
        asic3_sdio_base=0x0e000000;
        asic3_bus_shift=1;
    }
#endif
};

REGMACHINE(MachBlueangel)
