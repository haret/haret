// Definitions for Texas Instruments OMAP processors.
#include "machines.h" // Machine

class MachineOMAP : public Machine {
public:
    MachineOMAP();
    int detect();
};
