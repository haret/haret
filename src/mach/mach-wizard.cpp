#include "arch-omap.h" // MachineOMAP
#include "mach-types.h"
#include "memory.h" // memPhysSize

class MachWizard : public MachineOMAP {
public:
    MachWizard() {
        name = "HTC Wizard";
        OEMInfo[0] = L"WIZA100";
        OEMInfo[1] = L"WIZA200";
    }
};

REGMACHINE(MachWizard)
