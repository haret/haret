#include "machines.h"
#include "mach-types.h"

class MachAlpine : public MachinePXA27x {
public:
    MachAlpine() {
        name = "Alpine";
        OEMInfo[0] = L"PH10C";
        OEMInfo[1] = L"PH10D";
        machType = MACH_TYPE_HTCALPINE;
    }
};

REGMACHINE(MachAlpine)
