#include "arch-pxa.h" // MachinePXA27x
#include "mach-types.h"
#include "memory.h" // memPhysSize

class Macht700wx : public MachinePXA27x {
public:
    Macht700wx() {
        name = "Treo 700wx";
        OEMInfo[0] = L"Palm Treo 700w";
        machType = MACH_TYPE_T700WX;
    }
    void init() {
        memPhysSize=64*1024*1024;
    }
};

REGMACHINE(Macht700wx)
