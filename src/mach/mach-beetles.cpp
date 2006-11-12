#include "machines.h"
#include "mach-types.h"
//#include "asic3.h"

class MachBeetles : public MachinePXA27x {
public:
    MachBeetles() {
        name = "Beetles";
        OEMInfo[0] = L"hp iPAQ hw65";
        machType = MACH_TYPE_HTCBEETLES;
    }
#if 0
    void init() {
        asic3_gpio_base=0x10000000;
        asic3_sdio_base=0x0c000000;
        asic3_bus_shift=1;
    }
#endif
};

REGMACHINE(MachBeetles)
