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
struct irqData;
typedef void (*tracereporter)(irqData *, const char *, traceitem *);

// The layout of each item in the "trace buffer" shared between the
// exception handlers and the monitoring code.
struct traceitem {
    // Event specific reporter
    tracereporter reporter;
    // Data
    uint32 clock;
    uint32 d0, d1, d2, d3, d4;
};

// Maximum number of irq/trace level memory polls available.
static const uint32 MAX_MEMCHECK = 32;
// Maximum number of l1trace addresses available.
static const uint32 MAX_L1TRACE = 64;
// Maximum number of pc addresses that can be ignored.
static const uint32 MAX_IGNOREADDR = 64;

// Info on memory polling done in irq context.
class watchListVar;
struct pollinfo {
    class watchListVar *cls;
    uint32 count;
    memcheck list[MAX_MEMCHECK];
};

// Persistent data accessible by both exception handlers and regular
// code.
struct irqData {
    // Summary counters.
    uint32 irqCount, abortCount, prefetchCount;

    // Standard memory polling.
    struct pollinfo tracepoll;
    struct pollinfo irqpoll;
    struct pollinfo resumepoll;

    //
    // MMU tracing specific
    //
    uint32 *mmuVAddr;
    uint32 redirectVAddrBase;
    uint32 alterCount, alterVAddrs[MAX_L1TRACE];
    uint32 traceCount;
    memcheck traceAddrs[MAX_L1TRACE];
    uint32 max_l1trace, max_l1trace_after_resume;

    uint32 ignoreAddr[MAX_IGNOREADDR];
    uint32 ignoreAddrCount;

    //
    // PXA tracing specific
    //

    // Intel PXA based chip?
    int isPXA;

    // Instruction trace information.
    struct insn_s { uint32 addr1, addr2, reg1, reg2; } insns[2];
    uint32 dbr0, dbr1, dbcon;
    uint32 traceForWatch;

    //
    // Trace buffer.
    //
    uint32 overflows, errors;
    uint32 writePos, readPos;
    uint32 clock;
    uint32 exitEarly;
    struct traceitem traces[NR_TRACE];

    //
    // MMU L1 table merging (see mmumerge.cpp)
    //
    uint32 mergeTableCount;
    uint32 mergeTableStart;
    uint32 l1Copy[4096];
    uint32 l1Changed[4096];
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
    pos->clock = data->clock;
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
    DEF_GETCPRATTR(Name, Cpr, Op1, CRn, CRm, Op2, __irq,)
#define DEF_SETIRQCPR(Name, Cpr, Op1, CRn, CRm, Op2)     \
    DEF_SETCPRATTR(Name, Cpr, Op1, CRn, CRm, Op2, __irq,)

// Get pid - can't use haret's because it isn't in this irq section.
DEF_GETIRQCPR(get_PID, p15, 0, c13, c0, 0)

// Get the SPSR register
static inline uint32 __irq get_SPSR(void) {
    uint32 val;
    asm volatile("mrs %0, spsr" : "=r" (val));
    return val;
}

// Return the Modified Virtual Address (MVA) of a given address.
// (Can't use haret's because it needs to be in the __irq section.)
static inline uint32 __irq MVAddr_irq(uint32 pc) {
    if (pc <= 0x01ffffff)
        // Need to turn virtual address in to modified virtual address.
        pc |= (get_PID() & 0xfe000000);
    return pc;
}

// Is the "pc" in the list of ignored addresses?
static inline int __irq isIgnoredAddr(struct irqData *data, uint32 pc) {
    for (uint i=0; i<data->ignoreAddrCount; i++)
        if (pc == data->ignoreAddr[i])
            // This address is being ignored.
            return 1;
    return 0;
}


/****************************************************************
 * Intel PXA specific memory tracing (see pxatrace.cpp)
 ****************************************************************/

// The DBCON software debug register
DEF_SETIRQCPR(set_DBCON, p15, 0, c14, c4, 0)

void PXA_irq_handler(struct irqData *data, struct irqregs *regs);
int PXA_abort_handler(struct irqData *data, struct irqregs *regs);
int PXA_prefetch_handler(struct irqData *data, struct irqregs *regs);
void PXA_resume_handler(struct irqData *data, struct irqregs *regs);

void startPXAtraps(struct irqData *data);
void stopPXAtraps(struct irqData *data);
int prepPXAtraps(struct irqData *data);


/****************************************************************
 * MMU based memory tracing (see l1trace.cpp)
 ****************************************************************/

int L1_abort_handler(struct irqData *data, struct irqregs *regs);
int L1_prefetch_handler(struct irqData *data, struct irqregs *regs);
void L1_resume_handler(struct irqData *data, struct irqregs *regs);

void startL1traps(struct irqData *data);
void stopL1traps(struct irqData *data);
int prepL1traps(struct irqData *data);


/****************************************************************
 * MMU L1 table merging (see mmumerge.cpp)
 ****************************************************************/
void startMMUMerge(struct irqData *data);
void __irq stopMMUMerge(struct irqData *data);
void __irq checkMMUMerge(struct irqData *data);
int prepMMUMerge(struct irqData *data);
void dumpMMUMerge(struct irqData *data);

