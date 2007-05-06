// The virtual address of the irq vector
static const uint32 VADDR_PREFETCHOFFSET=0x0C;
static const uint32 VADDR_ABORTOFFSET=0x10;
static const uint32 VADDR_IRQOFFSET=0x18;

uint32 *findWinCEirq(uint32 offset);
uint32 hookResume(uint32 handler);
void unhookResume();
