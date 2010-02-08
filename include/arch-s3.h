//--------------------------------------------------------
// Definitions for Samsung s3c24xx chips.
#include "machines.h" // Machine

class MachineS3c2442 : public Machine {
public:
    MachineS3c2442();
    void init();
    int preHardwareShutdown();
    void hardwareShutdown(struct fbinfo *fbi);

    uint32 *channels, *uhcmap;
};

// XXX - assume they are the same for now.
class MachineS3c2440 : public MachineS3c2442 {
};
class MachineS3c2410 : public MachineS3c2442 {
};
int testS3C24xx();

//--------------------------------------------------------
// Definitions for Samsung s3c64xx chips.
class MachineS3c6400 : public Machine {
protected:
    void s3c6400ShutdownDMA(struct fbinfo *fbi);
    void resetUSB(volatile unsigned char * usb_sigmask);
    void clearIRQS(void);
public:
    MachineS3c6400();
    void init();
    int preHardwareShutdown();
    void hardwareShutdown(struct fbinfo *fbi);

    uint32 *usb_otgsfr, *dma_base[4];
    unsigned char *usb_sigmask;
};

// s3c6410 - assume they are the same for now.
class MachineS3c6410 : public MachineS3c6400 {
};

int testS3C64xx();
int testS3C();
