#include "arch-pxa.h" // MachinePXA
#include "mach-types.h"
//#include "asic3.h"

class MachH3900 : public MachinePXA {
public:
    MachH3900() {
        name = "H3900";
        OEMInfo[0] = L"Compaq iPAQ H39";
        machType = MACH_TYPE_H3900;
    }
};

REGMACHINE(MachH3900)
