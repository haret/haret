#include "arch-pxa.h" // MachinePXA27x
#include "mach-types.h"
#include "memory.h" // memPhysSize
//#include "asic3.h"

class MachHX4700 : public MachinePXA27x {
public:
    MachHX4700() {
        name = "HX4700";
        OEMInfo[0] = L"hp iPAQ hx47";
        OEMInfo[1] = L"HP iPAQ hx47";
        machType = MACH_TYPE_H4700;
    }
    void init() {
#if 0
        asic3_gpio_base=0x0c000000;
        asic3_sdio_base=0x0e000000;
        asic3_bus_shift=1;
#endif
        memPhysSize=64*1024*1024;
    }
};

REGMACHINE(MachHX4700)
