#include "machines.h"
#include "mach-types.h"
#include "memory.h"
//#include "asic3.h"

class MachSable : public MachinePXA27x {
public:
    MachSable() {
        name = "Sable";
        OEMInfo[0] = L"hp iPAQ hw69";
        machType = MACH_TYPE_HW6900;
    }
    void init() {
#if 0
        asic3_gpio_base=0x10000000;
        asic3_sdio_base=0x0c000000;
        asic3_bus_shift=1;
#endif
        memPhysSize=64*1024*1024;
    }
};

REGMACHINE(MachSable)
