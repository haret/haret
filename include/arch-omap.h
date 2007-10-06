// Definitions for Texas Instruments OMAP processors.
#include "arch-arm.h" // Machine926

class MachineOMAP850 : public Machine926 {
public:
    MachineOMAP850();
    void init();
    virtual int preHardwareShutdown();
    virtual void hardwareShutdown();
    uint8 *base;
};

class MachineOMAP15xx : public Machine {
public:
    MachineOMAP15xx();
    void init();
    virtual int preHardwareShutdown();
    virtual void hardwareShutdown();
    uint8 *base;
};
