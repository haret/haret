// Definitions for Intel StrongArm architecture.
#include "machines.h" // Machine

class MachineSA : public Machine {
public:
    MachineSA();
    int detect();
};
