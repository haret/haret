#include "machines.h"
#include "mach-types.h"

class MachMagician : public MachinePXA27x {
public:
    MachMagician() {
        name = "Magician";
        OEMInfo[0] = L"PM10";
        machType = MACH_TYPE_MAGICIAN;
    }
};

REGMACHINE(MachMagician)
