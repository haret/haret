// The virtual address of the irq vector
static const uint32 VADDR_PREFETCHOFFSET=0x0C;
static const uint32 VADDR_ABORTOFFSET=0x10;
static const uint32 VADDR_IRQOFFSET=0x18;

// Layout of an assembler function that can setup a C stack and entry
// point.  DO NOT CHANGE HERE without also upgrading the assembler
// code in asmstuff.S.
struct stackJumper_s {
    uint32 stack;
    uint32 data;
    uint32 execCode;
    uint32 returnCode;
    uint32 asm_handler[4];
};
extern struct stackJumper_s stackJumper;

uint32 *findWinCEirq(uint32 offset);
int hookResume(uint32 handler, uint32 stack, uint32 data, int complain=1);
void unhookResume();
