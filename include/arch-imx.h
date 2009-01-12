// Definitions for Freescale i.MX chips.
#ifndef _ARCH_IMX21_H
#define _ARCH_IMX21_H

#include "machines.h" // Machine

class MachineIMX21 : public Machine {
public:
    MachineIMX21();
    void init();
};

int testIMX();

#endif
