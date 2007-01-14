#include "arch-pxa.h" // MachinePXA
#include "mach-types.h"

class MachH1910 : public MachinePXA {
public:
    MachH1910() {
        name = "H1910";
        OEMInfo[0] = L"hp iPAQ h19";
        machType = MACH_TYPE_H1900;
    }
};

REGMACHINE(MachH1910)
