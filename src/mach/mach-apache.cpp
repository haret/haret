#include "machines.h"
#include "mach-types.h"
#include "memory.h"

class MachApache : public MachinePXA27x {
public:
    MachApache() {
        name = "HTC Apache";
        OEMInfo[0] = L"PA10";
        machType = MACH_TYPE_HTCAPACHE;
    }
    void init() {
        memPhysSize=64*1024*1024;
    }
};

REGMACHINE(MachApache)
