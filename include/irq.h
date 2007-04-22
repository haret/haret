//
// Code for working with interrupt watching and memory tracing.
//

#include "cpu.h" // DEF_GETCPRATTR
#include "cbitmap.h" // BITMAPSIZE
#include "watch.h" // memcheck

// Mark a function to be available during interrupt handling.
// Functions marked with this attribute will be relocated to an
// independant area of memory during interrupt watching.  Code in this
// section must not reference data or code in any other sections.
#define __irq __attribute__ ((__section__ (".text.irq")))


/****************************************************************
 * Shared storage between irq handlers and reporting code
 ****************************************************************/

// Number of items to place in the trace buffer.  MUST be a power of
// 2.
#define NR_TRACE 8192
#if (NR_TRACE & (NR_TRACE-1)) != 0
#error NR_TRACE must be a power of 2
#endif

struct traceitem;
typedef void (*tracereporter)(uint32 msecs, traceitem *);

// The layout of each item in the "trace buffer" shared between the
// exception handlers and the monitoring code.
struct traceitem {
    // Event specific reporter
    tracereporter reporter;
    // Data
    uint32 d0, d1, d2, d3, d4;
};

static const uint32 MAX_IRQ = 32 + 2 + 120;
static const uint32 MAX_IGNOREADDR = 64;
// Maximum number of irq/trace level memory polls available.
#define MAX_MEMCHECK 32

// Persistent data accessible by both exception handlers and regular
// code.
struct irqData {
    // Trace buffer.
    uint32 overflows, errors;
    uint32 writePos, readPos;
    struct traceitem traces[NR_TRACE];

    // Summary counters.
    uint32 irqCount, abortCount, prefetchCount;

    // Standard memory polling.
    uint32 tracepollcount;
    memcheck tracepolls[MAX_MEMCHECK];
    uint32 irqpollcount;
    memcheck irqpolls[MAX_MEMCHECK];

    //
    // MMU tracing specific
    //
    uint32 *alt_l1traceDesc, *l1traceDesc;
    uint32 alt_l1trace, l1trace;
    uint32 max_l1trace;

    //
    // PXA tracing specific
    //

    // Intel PXA based chip?
    int isPXA;

    // Irq information.
    uint8 *irq_ctrl, *gpio_ctrl;
    uint32 ignoredIrqs[BITMAPSIZE(MAX_IRQ)];
    uint32 demuxGPIOirq;

    // Debug information.
    uint32 ignoreAddr[MAX_IGNOREADDR];
    uint32 ignoreAddrCount;
    uint32 traceForWatch;

    // Instruction trace information.
    struct insn_s { uint32 addr1, addr2, reg1, reg2; } insns[2];
    uint32 dbr0, dbr1, dbcon;
};

// Add an item to the trace buffer.
static inline int __irq
add_trace(struct irqData *data, tracereporter reporter
          , uint32 d0=0, uint32 d1=0, uint32 d2=0, uint32 d3=0, uint32 d4=0)
{
    if (data->writePos - data->readPos >= NR_TRACE) {
        // No more space in trace buffer.
        data->overflows++;
        return -1;
    }
    struct traceitem *pos = &data->traces[data->writePos % NR_TRACE];
    pos->reporter = reporter;
    pos->d0 = d0;
    pos->d1 = d1;
    pos->d2 = d2;
    pos->d3 = d3;
    pos->d4 = d4;
    data->writePos++;
    return 0;
}


/****************************************************************
 * Misc declarations
 ****************************************************************/

int testWirqAvail();
int checkPolls(struct irqData *data, uint32 clock, memcheck *list, uint32 count);

// Contents of register description passed into the exception
// handlers.  This layout corresponds with the assembler code in
// irqchain.S.  If one changes a register it _will_ alter the machine
// state after the exception handler exits.
struct irqregs {
    union {
        struct {
            uint32 r0, r1, r2, r3, r4, r5, r6, r7, r8, r9, r10, r11, r12;
        };
        uint32 regs[13];
    };
    uint32 old_pc;
};

// Create a set of CPU coprocessor accessor functions for irq handlers
#define DEF_GETIRQCPR(Name, Cpr, Op1, CRn, CRm, Op2)     \
    DEF_GETCPRATTR(Name, Cpr, Op1, CRn, CRm, Op2, __irq)
#define DEF_SETIRQCPR(Name, Cpr, Op1, CRn, CRm, Op2)     \
    DEF_SETCPRATTR(Name, Cpr, Op1, CRn, CRm, Op2, __irq)

// Get pid - can't use haret's because it isn't in this irq section.
DEF_GETIRQCPR(get_PID, p15, 0, c13, c0, 0)

// Return the Modified Virtual Address (MVA) of a given PC.
static inline uint32 __irq transPC(uint32 pc) {
    if (pc <= 0x01ffffff)
        // Need to turn virtual address in to modified virtual address.
        pc |= (get_PID() & 0xfe000000);
    return pc;
}


/****************************************************************
 * Intel PXA specific memory tracing (see pxatrace.cpp)
 ****************************************************************/

void PXA_irq_handler(struct irqData *data, struct irqregs *regs);
int PXA_abort_handler(struct irqData *data, struct irqregs *regs);
int PXA_prefetch_handler(struct irqData *data, struct irqregs *regs);

void startPXAtraps(struct irqData *data);
void stopPXAtraps(struct irqData *data);
int prepPXAtraps(struct irqData *data);


/****************************************************************
 * MMU based memory tracing (see l1trace.cpp)
 ****************************************************************/

int L1_abort_handler(struct irqData *data, struct irqregs *regs);
int L1_prefetch_handler(struct irqData *data, struct irqregs *regs);

void startL1traps(struct irqData *data);
void stopL1traps(struct irqData *data);
int prepL1traps(struct irqData *data);
