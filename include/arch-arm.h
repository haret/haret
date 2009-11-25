// Definitions for ARM (the company) chips.
#ifndef _ARCH_ARM_H
#define _ARCH_ARM_H

#include "machines.h" // Machine

// Assembler functions
extern "C" {
    void cpuFlushCache_arm920();
    void cpuFlushCache_arm925();
    void cpuFlushCache_arm926();
    void cpuFlushCache_arm6();
    void cpuFlushCache_arm7();
}

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

class MachineArmV7 : public Machine {
public:
    MachineArmV7();
    int detect();
};

#endif // arch-arm.h
