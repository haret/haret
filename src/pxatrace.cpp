/* Code to use the Intel PXA's debug feature to trace software.
 *
 * (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <string.h> // memcpy

#include "memory.h" // memPhysMap
#include "script.h" // REG_VAR_INT
#include "arch-pxa.h" // testPXA
#include "output.h" // Output
#include "machines.h" // Mach
#include "arminsns.h" // getInsnName
#include "irq.h"

enum {
    ICHP_VAL_IRQ = 1<<31,

    START_GPIO_IRQS = 34,
    NR_GPIOx_IRQ = 10,

    IRQ_OFFSET = 0x40D00000,

    IRQ_ICHP_OFFSET = 0x18,
    IRQ_ICMR_OFFSET = 0x04,
    IRQ_ICIP_OFFSET = 0x00,
    IRQ_ICMR2_OFFSET = 0xA0,
    IRQ_ICIP2_OFFSET = 0x9c,

    GPIO_OFFSET = 0x40E00000,

    GPIO_GEDR0_OFFSET = 0x48,
    GPIO_GEDR1_OFFSET = 0x4c,
    GPIO_GEDR2_OFFSET = 0x50,
    GPIO_GEDR3_OFFSET = 0x148,
};

#define mask_ICHP_IRQ(ichp) (((ichp)>>16) & ((1<<6)-1))

// The CCNT performance monitoring register
DEF_GETIRQCPR(get_CCNT, p14, 0, c1, c1, 0)
// The DBCON software debug register
DEF_GETIRQCPR(get_DBCON, p15, 0, c14, c4, 0)
DEF_SETIRQCPR(set_DBCON, p15, 0, c14, c4, 0)
// Interrupt status register
DEF_GETIRQCPR(get_ICHP, p6, 0, c5, c0, 0)
// Set the IBCR0 software debug register
DEF_SETIRQCPR(set_IBCR0, p15, 0, c14, c8, 0)
// Set the IBCR1 software debug register
DEF_SETIRQCPR(set_IBCR1, p15, 0, c14, c9, 0)
// Set the EVTSEL performance monitoring register
DEF_SETIRQCPR(set_EVTSEL, p14, 0, c8, c1, 0)
// Set the INTEN performance monitoring register
DEF_SETIRQCPR(set_INTEN, p14, 0, c4, c1, 0)
// Set the PMNC performance monitoring register
DEF_SETIRQCPR(set_PMNC, p14, 0, c0, c1, 0)
// Set the DBR0 software debug register
DEF_SETIRQCPR(set_DBR0, p15, 0, c14, c0, 0)
// Set the DBR1 software debug register
DEF_SETIRQCPR(set_DBR1, p15, 0, c14, c3, 0)
// Set the DCSR software debug register
DEF_SETIRQCPR(set_DCSR, p14, 0, c10, c0, 0)
// Get the FSR software debug register
DEF_GETIRQCPR(get_FSR, p15, 0, c5, c0, 0)

// Enable CPU registers to catch insns and memory accesses
void __irq
startPXAtraps(struct irqData *data)
{
    if (! data->isPXA)
        return;
    // Enable performance monitor
    set_EVTSEL(0xffffffff);  // Disable explicit event counts
    set_INTEN(0);  // Don't use interrupts
    set_PMNC(0xf);  // Enable performance monitor; clear counter
    // Enable software debug
    if (data->dbcon || data->insns[0].addr1 != 0xFFFFFFFF) {
        set_DBCON(0);  // Clear DBCON
        set_DBR0(data->dbr0);
        set_DBR1(data->dbr1);
        set_DBCON(data->dbcon);
        set_DCSR(1<<31); // Global enable bit
        if (data->insns[0].addr1 != 0xFFFFFFFF)
            set_IBCR0(data->insns[0].addr1 | 0x01);
        if (data->insns[1].addr1 != 0xFFFFFFFF)
            set_IBCR1(data->insns[1].addr1 | 0x01);
    }
}

static void
report_winceResume(uint32 msecs, irqData *, traceitem *item)
{
    Output("%06d: %08x: cpu resumed", msecs, 0);
}

static void
report_irq(uint32 msecs, irqData *, traceitem *item)
{
    uint32 clock = item->d0, irq = item->d1;
    if (irq >= START_GPIO_IRQS)
        Output("%06d: %08x: irq %d(gpio %d)"
               , msecs, clock, irq, irq-START_GPIO_IRQS);
    else
        Output("%06d: %08x: irq %d(%s)"
               , msecs, clock, irq, Mach->getIrqName(irq));
}

void __irq
PXA_irq_handler(struct irqData *data, struct irqregs *regs)
{
    uint32 clock = get_CCNT();

    if (get_DBCON() != data->dbcon) {
        // Performance counter not running - reenable.
        add_trace(data, report_winceResume);
        startPXAtraps(data);
        clock = 0;
    }

    set_DBCON(0);
    uint32 irqs[2] = {
        (*(uint32*)&data->irq_ctrl[IRQ_ICIP_OFFSET]
         & *(uint32*)&data->irq_ctrl[IRQ_ICMR_OFFSET]),
        (*(uint32*)&data->irq_ctrl[IRQ_ICIP2_OFFSET]
         & *(uint32*)&data->irq_ctrl[IRQ_ICMR2_OFFSET])};
    for (int i=0; i<START_GPIO_IRQS; i++)
        if (TESTBIT(irqs, i) && !TESTBIT(data->ignoredIrqs, i))
            add_trace(data, report_irq, clock, i);
    if (irqs[0] & 0x400 && data->demuxGPIOirq) {
        // Gpio activity
        uint32 gpio_irqs[4] = {
            *(uint32*)&data->gpio_ctrl[GPIO_GEDR0_OFFSET],
            *(uint32*)&data->gpio_ctrl[GPIO_GEDR1_OFFSET],
            *(uint32*)&data->gpio_ctrl[GPIO_GEDR2_OFFSET],
            *(uint32*)&data->gpio_ctrl[GPIO_GEDR3_OFFSET]};
        for (int i=0; i<120; i++)
            if (TESTBIT(gpio_irqs, i) && !TESTBIT(data->ignoredIrqs
                                                  , START_GPIO_IRQS+i))
                add_trace(data, report_irq, clock, START_GPIO_IRQS+i);
    }

    // Irq time memory polling.
    checkPolls(data, clock, &data->irqpoll);
    // Trace time memory polling.
    checkPolls(data, clock, &data->tracepoll);
    set_DBCON(data->dbcon);
}

static void
report_memAccess(uint32 msecs, irqData *, traceitem *item)
{
    uint32 clock=item->d0, pc=item->d1, insn=item->d2, Rd=item->d3, Rn=item->d4;
    Output("%06d: %08x: debug %08x: %08x(%s) %08x %08x"
           , msecs, clock, pc, insn, getInsnName(insn), Rd, Rn);
}

// Code that handles memory access events.
int __irq
PXA_abort_handler(struct irqData *data, struct irqregs *regs)
{
    if ((get_FSR() & (1<<9)) == 0)
        // Not a debug trace event
        return 0;

    uint32 clock = get_CCNT();
    data->abortCount++;

    // Trace time memory polling.
    set_DBCON(0);
    int count = checkPolls(data, clock, &data->tracepoll);
    set_DBCON(data->dbcon);

    if (data->traceForWatch && !count)
        return 1;

    uint32 old_pc = MVAddr_irq(regs->old_pc - 8);
    if (isIgnoredAddr(data, old_pc))
        return 1;

    uint32 insn = *(uint32*)old_pc;
    add_trace(data, report_memAccess, clock, old_pc, insn
              , getReg(regs, mask_Rd(insn))
              , getReg(regs, mask_Rn(insn)));
    return 1;
}

static void
report_insnTrace(uint32 msecs, irqData *, traceitem *item)
{
    uint32 clock=item->d0, pc=item->d1, reg1=item->d2, reg2=item->d3;
    Output("%06d: %08x: insn %08x: %08x %08x"
           , msecs, clock, pc, reg1, reg2);
}

// Code that handles instruction breakpoint events.
int __irq
PXA_prefetch_handler(struct irqData *data, struct irqregs *regs)
{
    if ((get_FSR() & (1<<9)) == 0)
        // Not a debug trace event
        return 0;

    uint32 clock = get_CCNT();
    data->prefetchCount++;

    uint32 old_pc = MVAddr_irq(regs->old_pc-4);
    struct irqData::insn_s *idata = &data->insns[0];
    if (idata->addr1 == old_pc) {
        // Match on breakpoint.  Setup to single step next time.
        set_IBCR0(idata->addr2 | 0x01);
    } else if (idata->addr2 == old_pc) {
        // Called after single stepping - reset breakpoint.
        set_IBCR0(idata->addr1 | 0x01);
    } else {
        idata = &data->insns[1];
        if (idata->addr1 == old_pc) {
            // Match on breakpoint.  Setup to single step next time.
            set_IBCR1(idata->addr2 | 0x01);
        } else if (idata->addr2 == old_pc) {
            // Called after single stepping - reset breakpoint.
            set_IBCR1(idata->addr1 | 0x01);
        } else {
            // Huh?!  Got breakpoint for address not watched.
            data->errors++;
            set_IBCR0(0);
            set_IBCR1(0);
        }
    }

    add_trace(data, report_insnTrace, clock, old_pc
              , getReg(regs, idata->reg1)
              , getReg(regs, idata->reg2));

    // Trace time memory polling.
    set_DBCON(0);
    checkPolls(data, clock, &data->tracepoll);
    set_DBCON(data->dbcon);
    return 1;
}

// Reset CPU registers that conrol software debug / performance monitoring
void
stopPXAtraps(struct irqData *data)
{
    if (! data->isPXA)
        return;
    // Disable software debug
    set_IBCR0(0);
    set_IBCR1(0);
    set_DBCON(0);
    set_DCSR(0);
    // Disable performance monitor
    set_PMNC(0);
}

// Commands and variables are only applicable if AllocPhysMem is
// available and if this is a PXA based pda.
static int testPXAAvail() {
    return testWirqAvail() && testPXA();
}

// Mask of ignored interrupts (set in script.cpp)
static uint32 irqIgnore[BITMAPSIZE(MAX_IRQ)];
static uint32 irqDemuxGPIO = 1;
static uint32 traceForWatch;

REG_VAR_BITSET(testPXAAvail, "II", irqIgnore, MAX_IRQ,
               "The list of interrupts to ignore during WI")
REG_VAR_INT(testPXAAvail, "IRQGPIO", irqDemuxGPIO,
            "Turns on/off interrupt handler gpio irq demuxing")
REG_VAR_INT(testPXAAvail, "TRACEFORWATCH", traceForWatch,
            "Only report memory trace if ADDTRACEWATCH poll succeeds")

// Externally modifiable settings for software debug
static uint32 irqTrace = 0xFFFFFFFF;
static uint32 irqTraceMask = 0;
static uint32 irqTrace2 = 0xFFFFFFFF;
static uint32 irqTraceType = 2;
static uint32 irqTrace2Type = 2;

REG_VAR_INT(testPXAAvail, "TRACE", irqTrace,
            "Memory location to trace during WI")
REG_VAR_INT(testPXAAvail, "TRACEMASK", irqTraceMask,
            "Memory location mask to apply to TRACE during WI")
REG_VAR_INT(testPXAAvail, "TRACE2", irqTrace2,
            "Second memory location to trace during WI (only if no mask)")
REG_VAR_INT(testPXAAvail, "TRACETYPE", irqTraceType,
            "1=store only, 2=loads or stores, 3=loads only")
REG_VAR_INT(testPXAAvail, "TRACE2TYPE", irqTrace2Type,
            "1=store only, 2=loads or stores, 3=loads only")

// Externally modifiable settings for software tracing
static uint32 insnTrace = 0xFFFFFFFF, insnTraceReenable = 0xFFFFFFFF;
static uint32 insnTraceReg1 = 0, insnTraceReg2 = 1;
static uint32 insnTrace2 = 0xFFFFFFFF, insnTrace2Reenable = 0xFFFFFFFF;
static uint32 insnTrace2Reg1 = 0, insnTrace2Reg2 = 1;

REG_VAR_INT(testPXAAvail, "INSN", insnTrace,
            "Instruction address to monitor during WI")
REG_VAR_INT(testPXAAvail, "INSNREENABLE", insnTraceReenable,
            "Instruction address to reenable breakpoint after INSN")
REG_VAR_INT(testPXAAvail, "INSNREG1", insnTraceReg1,
            "Register to report during INSN breakpoint")
REG_VAR_INT(testPXAAvail, "INSNREG2", insnTraceReg2,
            "Second register to report during INSN breakpoint")
REG_VAR_INT(testPXAAvail, "INSN2", insnTrace2,
            "Second instruction address to monitor during WI")
REG_VAR_INT(testPXAAvail, "INSN2REENABLE", insnTrace2Reenable,
            "Instruction address to reenable breakpoint after INSN2")
REG_VAR_INT(testPXAAvail, "INSN2REG1", insnTrace2Reg1,
            "Register to report during INSN2 breakpoint")
REG_VAR_INT(testPXAAvail, "INSN2REG2", insnTrace2Reg2,
            "Second register to report during INSN2 breakpoint")

#define mask_DBCON_E0(val) (((val) & (0x3))<<0)
#define mask_DBCON_E1(val) (((val) & (0x3))<<2)
#define DBCON_MASKBIT (1<<8)

int
prepPXAtraps(struct irqData *data)
{
    data->isPXA = testPXA();
    if (! data->isPXA)
        return 0;
    // Check for software debug data watch points.
    if (irqTrace != 0xFFFFFFFF) {
        data->dbr0 = irqTrace;
        data->dbcon |= mask_DBCON_E0(irqTraceType);
        if (irqTraceMask) {
            data->dbr1 = irqTraceMask;
            data->dbcon |= DBCON_MASKBIT;
        } else if (irqTrace2 != 0xFFFFFFFF) {
            data->dbr1 = irqTrace2;
            data->dbcon |= mask_DBCON_E1(irqTrace2Type);
        }
    }

    // Setup instruction trace registers
    data->insns[0].addr1 = insnTrace;
    if (insnTraceReenable == 0xffffffff)
        data->insns[0].addr2 = insnTrace+4;
    else
        data->insns[0].addr2 = insnTraceReenable;
    data->insns[0].reg1 = insnTraceReg1;
    data->insns[0].reg2 = insnTraceReg2;
    data->insns[1].addr1 = insnTrace2;
    if (insnTrace2Reenable == 0xffffffff)
        data->insns[1].addr2 = insnTrace2+4;
    else
        data->insns[1].addr2 = insnTrace2Reenable;
    data->insns[1].reg1 = insnTrace2Reg1;
    data->insns[1].reg2 = insnTrace2Reg2;

    if (insnTrace != 0xFFFFFFFF || irqTrace != 0xFFFFFFFF) {
        Output("Will set memory tracing to:%08x %08x %08x %08x %08x"
               , data->dbr0, data->dbr1, data->dbcon
               , irqTrace, irqTrace2);
        Output("Will set software debug to:%08x->%08x %08x->%08x"
               , data->insns[0].addr1, data->insns[0].addr2
               , data->insns[1].addr1, data->insns[1].addr2);
    }

    data->gpio_ctrl = memPhysMap(GPIO_OFFSET);
    data->irq_ctrl = memPhysMap(IRQ_OFFSET);
    memcpy(data->ignoredIrqs, irqIgnore, sizeof(irqIgnore));
    data->demuxGPIOirq = irqDemuxGPIO;
    data->traceForWatch = traceForWatch;

    return 0;
}
