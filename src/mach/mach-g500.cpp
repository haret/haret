#include "arch-s3.h" // MachineS3
#include "mach-types.h"
#include "memory.h" // memPhysSize

class MachG500 : public MachineS3 {
public:
    MachG500() {
        name = "G500";
        OEMInfo[0] = L"G50V";
        machType = MACH_TYPE_G500;
    }
    void init() {
        MachineS3::init();
        memPhysSize = 64*1024*1024;
    }
};

REGMACHINE(MachG500)
