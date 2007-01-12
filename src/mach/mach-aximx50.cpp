#include "arch-pxa.h" // MachinePXA27x
#include "mach-types.h"
#include "memory.h" // memPhysSize

class MachAximX50 : public MachinePXA27x {
public:
    MachAximX50() {
        name = "AximX50/51";
        OEMInfo[0] = L"Dell Axim X50";
        OEMInfo[1] = L"Dell Axim X51";
        machType = MACH_TYPE_X50;
    }
    void init() {
	memPhysAddr = 0xa8000000;
        memPhysSize = 64*1024*1024;
    }
};

REGMACHINE(MachAximX50)
