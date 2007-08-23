#ifndef _ARCH_ARM_H
#define _ARCH_ARM_H

// Definitions for ARM (the company) chips.
#include "machines.h" // Machine

class Machine920t : public Machine {
public:
    Machine920t();
    int detect();
};

class Machine926 : public Machine {
public:
    Machine926();
    int detect();
};

class MachineArmV6 : public Machine {
public:
    MachineArmV6();
    int detect();
};

#endif // arch-arm.h
