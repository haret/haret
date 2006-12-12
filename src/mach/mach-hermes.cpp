#include "arch-s3.h" // MachineS3
#include "mach-types.h"
#include "memory.h" // memPhysSize

class MachHermes : public MachineS3 {
public:
    MachHermes() {
        name = "HTC Hermes";
        OEMInfo[0] = L"HERM100";
        OEMInfo[1] = L"HERM200";
        OEMInfo[2] = L"HERM300";
        machType = MACH_TYPE_HTCHERMES;
    }
};

REGMACHINE(MachHermes)
