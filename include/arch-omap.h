// Definitions for Texas Instruments OMAP processors.
#include "arch-arm.h" // Machine926

class MachineOMAP850 : public Machine926 {
public:
    MachineOMAP850();
    void init();
    virtual int preHardwareShutdown();
    virtual void hardwareShutdown();
    uint16 *dma;
};
