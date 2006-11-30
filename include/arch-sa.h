// Definitions for Intel StrongArm architecture.
#include "machines.h" // Machine

// PXA
class MachineSA : public Machine {
public:
    MachineSA();
    int detect();
};
