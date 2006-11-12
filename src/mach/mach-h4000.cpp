#include "machines.h"
#include "mach-types.h"
//#include "asic3.h"

class MachH4000 : public MachinePXA27x {
public:
    MachH4000() {
        name = "H4000";
        OEMInfo[0] = L"hp iPAQ h41";
        OEMInfo[1] = L"hp iPAQ h43";
        machType = MACH_TYPE_H4000;
    }
#if 0
    void init() {
        asic3_gpio_base=0x0c000000;
        asic3_sdio_base=0x10000000;
        asic3_bus_shift=2;
    }
#endif
};

REGMACHINE(MachH4000)
