// Definitions for Texas Instruments OMAP processors.
#include "machines.h" // Machine

class MachineMSM7xxxA : public Machine {
public:
    MachineMSM7xxxA();
    void init();
};

class MachineMSM7xxx : public Machine {
public:
    MachineMSM7xxx();
    void init();
};

class MachineQSD8xxx : public Machine {
protected:
    void configureFb(struct fbinfo *);
public:
    MachineQSD8xxx();
    void init();
    int preHardwareShutdown(struct fbinfo *);
    void hardwareShutdown(struct fbinfo *);
};

// Aliases
class MachineMSM7201A : public MachineMSM7xxxA {
};

class MachineMSM7200 : public MachineMSM7xxx {
};
class MachineMSM7500 : public MachineMSM7xxx {
};
