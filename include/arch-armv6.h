#ifndef _ARCH_ARMV6_H
#define _ARCH_ARMV6_H

// Definitions for ARM 920t chips.
#include "machines.h" // Machine

class MachineArmV6 : public Machine {
public:
    MachineArmV6();
    int detect();
};

#endif // arch-armv6.h
