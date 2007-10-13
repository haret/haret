#include "machines.h" // Machine

// PXA 25x and 26x
class MachinePXA : public Machine {
public:
    MachinePXA();
    int detect();
    void init();
    virtual int preHardwareShutdown();
    virtual void hardwareShutdown(struct fbinfo *fbi);

    uint32 dcsr_count;
    uint32 *dma, *udc;
};

// PXA 27x
class MachinePXA27x : public MachinePXA {
public:
    MachinePXA27x();
    int detect();
    void init();
    virtual int preHardwareShutdown();
    virtual void hardwareShutdown(struct fbinfo *fbi);

    uint32 *cken, *uhccoms;
};

int testPXA();
