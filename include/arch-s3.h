// Definitions for Samsung s3c24xx chips.
#include "arch-920t.h" // Machine920t

class MachineS3 : public Machine920t {
public:
    MachineS3();
    int detect();
    void init();
    int preHardwareShutdown();
    void hardwareShutdown();

    uint32 *channels, *uhcmap;
};
