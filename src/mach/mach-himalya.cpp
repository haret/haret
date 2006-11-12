#include "machines.h"
#include "mach-types.h"
//#include "asic3.h"

class MachHimalaya : public MachinePXA {
public:
    MachHimalaya() {
        name = "Himalaya";
        OEMInfo[0] = L"PH10A";
        OEMInfo[1] = L"PH10B";
        machType = MACH_TYPE_HIMALAYA;
    }
#if 0
    void init() {
        asic3_gpio_base=0x0d800000;
        asic3_sdio_base=0x0c800000;
        asic3_bus_shift=2;
    }
    int getBoardID() {
        return (asic3gpioGetState(('C'-'A')*16+5)*4+
                asic3gpioGetState(('C'-'A')*16+4)*2+
                asic3gpioGetState(('C'-'A')*16+3));
    }
#endif
};

REGMACHINE(MachHimalaya)
