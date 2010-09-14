// Definitions for Texas Instruments OMAP processors.
#include "machines.h" // Machine

extern "C" {
    void bootQSD8xxx(char *kernel, uint32 machtype, char *tags);
}

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
    void shutdownInterrupts();
    void shutdownTimers();
    void shutdownSirc();
    void shutdownDMA();
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
