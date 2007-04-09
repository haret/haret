/* Monitor WinCE exceptions.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <windows.h> // for pkfuncs.h
#include "pkfuncs.h" // AllocPhysMem
#include <time.h> // time
#include <string.h> // memcpy

#include "xtypes.h"
#include "watch.h" // memcheck
#include "output.h" // Output
#include "memory.h" // memPhysMap
#include "script.h" // REG_CMD
#include "machines.h" // Mach
#include "arch-pxa.h" // MachinePXA
#include "lateload.h" // LATE_LOAD
#include "irq.h"

LATE_LOAD(AllocPhysMem, "coredll")
LATE_LOAD(FreePhysMem, "coredll")


/****************************************************************
 * C part of exception handlers
 ****************************************************************/

static void
report_memPoll(uint32 msecs, traceitem *item)
{
    memcheck *mc = (memcheck*)item->d0;
    uint32 clock=item->d1, val=item->d2, mask=item->d3;
    mc->reporter(msecs, clock, mc, val, mask);
}

// Perform a set of memory polls and add to trace buffer.
int __irq
checkPolls(struct irqData *data, uint32 clock, memcheck *list, uint32 count)
{
    int foundcount = 0;
    for (uint i=0; i<count; i++) {
        memcheck *mc = &list[i];
        uint32 val, maskval;
        int ret = testMem(mc, &val, &maskval);
        if (!ret)
            continue;
        foundcount++;
        ret = add_trace(data, report_memPoll, (uint32)mc, clock, val, maskval);
        if (ret)
            // Couldn't add trace - reset compare function.
            mc->trySuppress = 0;
    }
    return foundcount;
}

// Handler for interrupt events.  Note that this is running in
// "Modified Virtual Address" mode, so avoid reading any global
// variables or calling any non local functions.
extern "C" void __irq
irq_handler(struct irqData *data, struct irqregs *regs)
{
    data->irqCount++;
    if (data->isPXA) {
        // Separate routine for PXA chips
        PXA_irq_handler(data, regs);
        return;
    }

    // Irq time memory polling.
    checkPolls(data, 0, data->irqpolls, data->irqpollcount);
    // Trace time memory polling.
    checkPolls(data, 0, data->tracepolls, data->tracepollcount);
}

extern "C" int __irq
abort_handler(struct irqData *data, struct irqregs *regs)
{
    data->abortCount++;
    if (data->isPXA) {
        // Separate routine for PXA chips
        int ret = PXA_abort_handler(data, regs);
        if (ret)
            return ret;
    }
    return L1_abort_handler(data, regs);
}

extern "C" int __irq
prefetch_handler(struct irqData *data, struct irqregs *regs)
{
    data->prefetchCount++;
    if (data->isPXA) {
        // Separate routine for PXA chips
        int ret = PXA_prefetch_handler(data, regs);
        if (ret)
            return ret;
    }
    return L1_prefetch_handler(data, regs);
}


/****************************************************************
 * Standard interface commands and variables
 ****************************************************************/

// Commands and variables are only applicable if AllocPhysMem is
// available.
int testWirqAvail() {
    return late_AllocPhysMem && late_FreePhysMem;
}

static uint32 watchirqcount;
static memcheck watchirqpolls[16];

static void
cmd_addirqwatch(const char *cmd, const char *args)
{
    watchCmdHelper(watchirqpolls, ARRAY_SIZE(watchirqpolls), &watchirqcount
                   , cmd, args);
}
REG_CMD(testWirqAvail, "ADDIRQWATCH", cmd_addirqwatch,
        "ADDIRQWATCH <addr> [<mask> <32|16|8> <cmpValue>]\n"
        "  Setup an address to be polled when an irq hits\n"
        "  See ADDWATCH for syntax.  <CLEAR|LS>IRQWATCH is also available.")
REG_CMD_ALT(testWirqAvail, "CLEARIRQWATCH", cmd_addirqwatch, clear, 0)
REG_CMD_ALT(testWirqAvail, "LSIRQWATCH", cmd_addirqwatch, list, 0)

static uint32 watchtracecount;
static memcheck watchtracepolls[16];

static void
cmd_addtracewatch(const char *cmd, const char *args)
{
    watchCmdHelper(watchtracepolls, ARRAY_SIZE(watchtracepolls), &watchtracecount
                   , cmd, args);
}
REG_CMD(testWirqAvail, "ADDTRACEWATCH", cmd_addtracewatch,
        "ADDTRACEWATCH <addr> [<mask> <32|16|8> <cmpValue>]\n"
        "  Setup an address to be polled when an irq hits\n"
        "  See ADDWATCH for syntax.  <CLEAR|LS>TRACEWATCH is also available.")
REG_CMD_ALT(testWirqAvail, "CLEARTRACEWATCH", cmd_addtracewatch, clear, 0)
REG_CMD_ALT(testWirqAvail, "LSTRACEWATCH", cmd_addtracewatch, list, 0)


/****************************************************************
 * Code to report feedback from exception handlers
 ****************************************************************/

static uint32 LastOverflowReport;

// Pull a trace event from the trace buffer and print it out.  Returns
// 0 if nothing was available.
static int
printTrace(uint32 msecs, struct irqData *data)
{
    uint32 writePos = data->writePos;
    if (data->readPos == writePos)
        return 0;
    uint32 tmpoverflow = data->overflows;
    if (tmpoverflow != LastOverflowReport) {
        Output("overflowed %d traces"
               , tmpoverflow - LastOverflowReport);
        LastOverflowReport = tmpoverflow;
    }
    struct traceitem *cur = &data->traces[data->readPos % NR_TRACE];
    cur->reporter(msecs, cur);
    data->readPos++;
    return 1;
}

// Called before exceptions are taken over.
static void
preLoop(struct irqData *data)
{
    LastOverflowReport = 0;

    // Setup memory tracing.
    memcpy(data->irqpolls, watchirqpolls, sizeof(data->irqpolls));
    data->irqpollcount = watchirqcount;
    memcpy(data->tracepolls, watchtracepolls, sizeof(data->tracepolls));
    data->tracepollcount = watchtracecount;
}

// Code called while exceptions are rerouted - should return after
// 'seconds' amount of time has passed.  This is called from normal
// process context, and the full range of input/output functions and
// variables are available.
static void
mainLoop(struct irqData *data, int seconds)
{
    uint32 start_time = GetTickCount();
    uint32 cur_time = start_time;
    uint32 fin_time = cur_time + seconds * 1000;
    int tmpcount = 0;
    while (cur_time <= fin_time) {
        int ret = printTrace(cur_time - start_time, data);
        if (ret) {
            // Processed a trace - try to process another without
            // sleeping.
            tmpcount++;
            if (tmpcount < 100)
                continue;
            // Hrmm.  Recheck the current time so that we don't run
            // away reporting traces.
        } else
            // Nothing to report; yield the cpu.
            late_SleepTillTick();
        cur_time = GetTickCount();
        tmpcount = 0;
    }
}

// Called after exceptions are restored to wince - may be used to
// report additional data or cleanup structures.
static void
postLoop(struct irqData *data)
{
    // Flush trace buffer.
    for (;;) {
        int ret = printTrace(0, data);
        if (! ret)
            break;
    }
    Output("Handled %d irq, %d abort, %d prefetch, %d lost, %d errors"
           , data->irqCount, data->abortCount, data->prefetchCount
           , data->overflows, data->errors);
}


/****************************************************************
 * Binding of "chained" irq handler
 ****************************************************************/

// Layout of memory in physically continuous ram.
static const int IRQ_STACK_SIZE = 4096;
struct irqChainCode {
    // Stack for C prefetch code.
    char stack_prefetch[IRQ_STACK_SIZE];
    // Stack for C abort code.
    char stack_abort[IRQ_STACK_SIZE];
    // Stack for C irq code.
    char stack_irq[IRQ_STACK_SIZE];
    // Data for C code.
    struct irqData data;

    // Variable length array storing asm/C exception handler code.
    char cCode[1] PAGE_ALIGNED;
};

// Low level information to be passed to the assembler part of the
// chained exception handler.  DO NOT CHANGE HERE without also
// upgrading the assembler code.
struct irqAsmVars {
    // Modified Virtual Address of irqData data.
    uint32 dataMVA;
    // Standard WinCE interrupt handler.
    uint32 winceIrqHandler;
    // Standard WinCE abort handler.
    uint32 winceAbortHandler;
    // Standard WinCE prefetch handler.
    uint32 wincePrefetchHandler;
};

extern "C" {
    // Symbols added by linker.
    extern char irq_start;
    extern char irq_end;

    // Assembler linkage.
    extern char asmIrqVars;
    extern void irq_chained_handler();
    extern void abort_chained_handler();
    extern void prefetch_chained_handler();
}
#define offset_asmIrqVars() (&asmIrqVars - &irq_start)
#define offset_asmIrqHandler() ((char *)irq_chained_handler - &irq_start)
#define offset_asmAbortHandler() ((char *)abort_chained_handler - &irq_start)
#define offset_asmPrefetchHandler() ((char *)prefetch_chained_handler - &irq_start)
#define size_cHandlers() (&irq_end - &irq_start)
#define size_handlerCode() (uint)(&((irqChainCode*)0)->cCode[size_cHandlers()])

// The virtual address of the irq vector
static const uint32 VADDR_IRQTABLE=0xffff0000;
static const uint32 VADDR_PREFETCHOFFSET=0x0C;
static const uint32 VADDR_ABORTOFFSET=0x10;
static const uint32 VADDR_IRQOFFSET=0x18;

// Locate a WinCE exception handler.  This assumes the handler is
// setup in a manor that wince has been observed to do in the past.
static uint32 *
findWinCEirq(uint8 *irq_table, uint32 offset)
{
    uint32 irq_ins = *(uint32*)&irq_table[offset];
    if ((irq_ins & 0xfffff000) != 0xe59ff000) {
        // We only know how to handle LDR PC, #XXX instructions.
        Output(C_INFO "Unknown irq instruction %08x", irq_ins);
        return NULL;
    }
    uint32 ins_offset = (irq_ins & 0xFFF) + 8;
    //Output("Ins=%08x ins_offset=%08x", irq_ins, ins_offset);
    return (uint32 *)(&irq_table[offset + ins_offset]);
}

// Main "watch irq" command entry point.
static void
cmd_wirq(const char *cmd, const char *args)
{
    uint32 seconds;
    if (!get_expression(&args, &seconds)) {
        Output(C_ERROR "line %d: Expected <seconds>", ScriptLine);
        return;
    }

    // Map in the IRQ vector tables in a place that we can be sure
    // there is full read/write access to it.
    uint8 *irq_table = memPhysMap(memVirtToPhys(VADDR_IRQTABLE));

    // Locate position of wince exception handlers.
    uint32 *irq_loc = findWinCEirq(irq_table, VADDR_IRQOFFSET);
    if (!irq_loc)
        return;
    uint32 *abort_loc = findWinCEirq(irq_table, VADDR_ABORTOFFSET);
    if (!abort_loc)
        return;
    uint32 *prefetch_loc = findWinCEirq(irq_table, VADDR_PREFETCHOFFSET);
    if (!prefetch_loc)
        return;
    uint32 newIrqHandler, newAbortHandler, newPrefetchHandler;

    // Allocate space for the irq handlers in physically continuous ram.
    void *rawCode = 0;
    ulong dummy;
    rawCode = late_AllocPhysMem(size_handlerCode()
                                , PAGE_EXECUTE_READWRITE, 0, 0, &dummy);
    irqChainCode *code = (irqChainCode *)cachedMVA(rawCode);
    int ret;
    struct irqData *data = &code->data;
    struct irqAsmVars *asmVars = (irqAsmVars*)&code->cCode[offset_asmIrqVars()];
    if (!rawCode) {
        Output(C_INFO "Can't allocate memory for irq code");
        goto abort;
    }
    if (!code) {
        Output(C_INFO "Can't find vm addr of alloc'd physical ram.");
        goto abort;
    }
    memset(code, 0, size_handlerCode());

    // Copy the C handlers to alloc'd space.
    memcpy(code->cCode, &irq_start, size_cHandlers());

    // Setup the assembler links
    asmVars->dataMVA = (uint32)data;
    asmVars->winceIrqHandler = *irq_loc;
    asmVars->winceAbortHandler = *abort_loc;
    asmVars->wincePrefetchHandler = *prefetch_loc;
    newIrqHandler = (uint32)&code->cCode[offset_asmIrqHandler()];
    newAbortHandler = (uint32)&code->cCode[offset_asmAbortHandler()];
    newPrefetchHandler = (uint32)&code->cCode[offset_asmPrefetchHandler()];

    Output("irq:%08x@%p=%08x abort:%08x@%p=%08x"
           " prefetch:%08x@%p=%08x"
           " data=%08x sizes=c:%08x,t:%08x"
           , asmVars->winceIrqHandler, irq_loc, newIrqHandler
           , asmVars->winceAbortHandler, abort_loc, newAbortHandler
           , asmVars->wincePrefetchHandler, prefetch_loc, newPrefetchHandler
           , asmVars->dataMVA
           , size_cHandlers(), size_handlerCode());

    ret = prepPXAtraps(data);
    if (ret)
        goto abort;

    preLoop(data);

    ret = prepL1traps(data);
    if (ret)
        goto abort;

    // Replace old handler with new handler.
    Output("Replacing windows exception handlers...");
    take_control();
    startPXAtraps(data);
    startL1traps(data);
    Mach->flushCache();
    *irq_loc = newIrqHandler;
    *abort_loc = newAbortHandler;
    *prefetch_loc = newPrefetchHandler;
    return_control();
    Output("Finished installing exception handlers.");

    // Loop till time up.
    mainLoop(data, seconds);

    // Restore wince handler.
    Output("Restoring windows exception handlers...");
    take_control();
    stopPXAtraps(data);
    stopL1traps(data);
    Mach->flushCache();
    *irq_loc = asmVars->winceIrqHandler;
    *abort_loc = asmVars->winceAbortHandler;
    *prefetch_loc = asmVars->wincePrefetchHandler;
    return_control();
    Output("Finished restoring windows exception handlers.");

    postLoop(data);
abort:
    if (rawCode)
        late_FreePhysMem(rawCode);
}
REG_CMD(testWirqAvail, "WI|RQ", cmd_wirq,
        "WIRQ <seconds>\n"
        "  Watch which IRQ occurs for some period of time and report them.")
