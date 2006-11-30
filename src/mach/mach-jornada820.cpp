#include "arch-sa.h" // MachineSA
#include "mach-types.h" // MACH_TYPE_JORNADA820

class MachJornada820 : public MachineSA {
public:
    MachJornada820() {
        name = "Jornada820";
        PlatformType = L"Jupiter";
        OEMInfo[0] = L"HP, Jornada 820";
        machType = MACH_TYPE_JORNADA820;
    }
};

REGMACHINE(MachJornada820)
