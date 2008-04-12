// Definitions for Texas Instruments OMAP processors.
#include "machines.h" // Machine

class MachineMSM7500 : public Machine {
public:
    MachineMSM7500();
    void init();
};

// XXX - assume they are the same for now.
class MachineMSM7200 : public MachineMSM7500 {
};
