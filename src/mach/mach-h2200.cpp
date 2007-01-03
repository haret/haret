#include "arch-pxa.h" // MachinePXA
#include "mach-types.h"

class MachH2200 : public MachinePXA {
public:
    MachH2200() {
        name = "H2200";
        OEMInfo[0] = L"hp iPAQ h22";
        machType = MACH_TYPE_H2200;
    }
};

REGMACHINE(MachH2200)
