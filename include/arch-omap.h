// Definitions for Texas Instruments OMAP processors.
#include "machines.h" // Machine

class MachineOMAP850 : public Machine {
public:
    MachineOMAP850();
    void init();
    virtual int preHardwareShutdown(struct fbinfo *fbi);
    virtual void hardwareShutdown(struct fbinfo *fbi);
    uint8 *base;
};

class MachineOMAP15xx : public Machine {
public:
    MachineOMAP15xx();
    void init();
    virtual int preHardwareShutdown(struct fbinfo *fbi);
    virtual void hardwareShutdown(struct fbinfo *fbi);
    uint8 *base;
};
