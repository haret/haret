#include <windows.h>
#include "arch-pxa.h"
#include "mach-types.h"

class MachAximX5 : public MachinePXA {
public:
    MachAximX5() {
        name = "AximX5";
        OEMInfo[0] = L"Dell Axim X5";
        machType = MACH_TYPE_AXIM;
    }
};

REGMACHINE(MachAximX5)
