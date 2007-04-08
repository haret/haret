// Definitions for manipulating ARM instructions

struct extraregs {
    union {
        struct {
            uint32 r13, r14;
        };
        uint32 regs[2];
    };
    uint32 didfetch;
};

// Extract the Rd/Rn register numbers from an ARM instruction.
#define mask_Rn(insn) (((insn)>>16) & 0xf)
#define mask_Rd(insn) (((insn)>>12) & 0xf)

uint32 getReg(struct irqregs *regs, struct extraregs *er, uint32 nr);
const char *getInsnName(uint32 insn);
