#include "machines.h"
#include "mach-types.h"
#include "memory.h"
//#include "asic3.h"

class MachH4700 : public MachinePXA27x {
public:
    MachH4700() {
        name = "H4700";
        OEMInfo[0] = L"hp iPAQ hx47";
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

REGMACHINE(MachH4700)
