// Definitions for Texas Instruments OMAP processors.
#include "arch-arm.h" // Machine926

class MachineMSM7500 : public MachineArmV6 {
public:
    MachineMSM7500();
    void init();
};

// XXX - assume they are the same for now.
class MachineMSM7200 : public MachineMSM7500 {
};
