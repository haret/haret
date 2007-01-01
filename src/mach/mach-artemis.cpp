#include "arch-omap.h" // MachineOMAP
#include "mach-types.h"
#include "memory.h" // memPhysSize

class MachArtemis : public MachineOMAP {
public:
    MachArtemis() {
        name = "HTC Artemis";
        OEMInfo[0] = L"ARTE";
    }
};

REGMACHINE(MachArtemis)
