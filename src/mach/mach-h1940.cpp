#include "arch-s3.h" // MachineS3
#include "mach-types.h"
#include "memory.h" // memPhysSize

class MachH1940 : public MachineS3 {
public:
    MachH1940() {
        name = "H1940";
        OEMInfo[0] = L"HP iPAQ h193";
        OEMInfo[1] = L"HP iPAQ h194";
        machType = MACH_TYPE_H1940;
    }
    void init() {
        MachineS3::init();
        memPhysSize = 64*1024*1024;
    }
};

REGMACHINE(MachH1940)
