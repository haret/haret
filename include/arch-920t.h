#ifndef _ARCH_920T_H
#define _ARCH_920T_H

// Definitions for ARM 920t chips.
#include "machines.h" // Machine

class Machine920t : public Machine {
public:
    Machine920t();
    int detect();
};

#endif // arch-920t.h
