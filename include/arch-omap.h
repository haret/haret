// Definitions for Texas Instruments OMAP processors.
#include "arch-920t.h" // Machine920t

class MachineOMAP850 : public Machine920t {
public:
    MachineOMAP850();
    int detect();
    void init();
};
