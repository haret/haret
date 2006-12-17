// Definitions for Samsung s3c24xx chips.
#include "machines.h" // Machine

class MachineS3 : public Machine {
public:
    MachineS3();
    int detect();
    void init();
    int preHardwareShutdown();
    void hardwareShutdown();

    uint32 *channels;
};
