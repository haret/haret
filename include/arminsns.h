// Definitions for manipulating ARM instructions

// Extract the Rd/Rn register numbers from an ARM instruction.
#define mask_Rn(insn) (((insn)>>16) & 0xf)
#define mask_Rd(insn) (((insn)>>12) & 0xf)

uint32 getReg(struct irqregs *regs, uint32 nr);
void setReg(struct irqregs *regs, uint32 nr, uint32 val);
const char *getInsnName(uint32 insn);
