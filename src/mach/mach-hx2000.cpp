#include "arch-pxa.h" // MachinePXA27x
#include "mach-types.h"
#include "memory.h" // memPhysSize

class MachHX2000 : public MachinePXA27x {
public:
    MachHX2000() {
        name = "HX2000";
        OEMInfo[0] = L"HP iPAQ hx2";
        machType = MACH_TYPE_HX2750;
    }
    void init() {
        memPhysSize=64*1024*1024;
    }
};

REGMACHINE(MachHX2000)
