#include "arch-pxa.h" // MachinePXA
#include "mach-types.h"
//#include "asic3.h"

class MachH5000 : public MachinePXA {
public:
    MachH5000() {
        name = "H5000";
        OEMInfo[0] = L"HP iPAQ h5";
        OEMInfo[1] = L"hp iPAQ h5";
        machType = MACH_TYPE_H5400;
    }
};

REGMACHINE(MachH5000)
