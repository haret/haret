/* Alter MMU table to catch reads/writes to memory.
 *
 * (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include "windows.h"
#include "pkfuncs.h" // VirtualCopy

#include "script.h" // REG_VAR_INT
#include "output.h" // Output
#include "memory.h" // MMU_L1_UNMAPPED
#include "arminsns.h" // getReg
#include "irq.h"

#define ONEMEG (1024*1024)
#define TOPBITS 0xFFF00000
#define BOTBITS 0x000FFFFF

void
startL1traps(struct irqData *data)
{
    if (!data->l1traceDesc)
        return;
    // Unmap the l1 mmu reference
    *data->alt_l1traceDesc = *data->l1traceDesc;
    *data->l1traceDesc = MMU_L1_UNMAPPED;
    // XXX - make sure unbuffered/uncached/read+writable
}

void __irq
stopL1traps(struct irqData *data)
{
    if (!data->l1traceDesc)
        return;
    // Restore the previous l1 mmu info.
    *data->l1traceDesc = *data->alt_l1traceDesc;
    *data->alt_l1traceDesc = MMU_L1_UNMAPPED;
    data->l1traceDesc = NULL;
}

// Flush the I and D TLBs.
DEF_SETIRQCPR(set_TLBflush, p15, 0, c8, c7, 0)

static void
report_giveup(uint32 msecs, traceitem *item)
{
    Output("%06d: %08x: giving up - clearing mapping"
           , msecs, 0);
}

static void __irq
giveUp(struct irqData *data, struct irqregs *regs, uint32 addr)
{
    add_trace(data, report_giveup);
    data->errors++;
    stopL1traps(data);
    // Rerun instruction on return to user code.
    regs->old_pc -= 4;
    set_TLBflush(0);
}

static void
report_memAccess(uint32 msecs, traceitem *item)
{
    uint32 addr=item->d0, pc=item->d1, insn=item->d2, val=item->d3;
    Output("%06d: debug %08x: %08x(%s) %08x %08x"
           , msecs, pc, insn, getInsnName(insn), val, addr);
}

static void __irq
tryEmulate(struct irqData *data, struct irqregs *regs, uint32 addr)
{
    uint32 old_pc = transPC(regs->old_pc - 8);
    uint32 insn = *(uint32*)old_pc;
    uint32 newaddr = data->alt_l1trace | (addr & BOTBITS);

    if (--data->max_l1trace == 0)
        goto fail;

#define Ibit(insn) ((insn)&(1<<25))
#define Pbit(insn) ((insn)&(1<<24))
#define Ubit(insn) ((insn)&(1<<23))
#define Bbit(insn) ((insn)&(1<<22))
#define Wbit(insn) ((insn)&(1<<21))
#define Lbit(insn) ((insn)&(1<<20))
#define Sbit(insn) ((insn)&(1<<6))
#define Hbit(insn) ((insn)&(1<<5))

    uint32 val;
    if ((insn & 0x0C000000) == 0x04000000) {
        if (Pbit(insn) == 0)
            goto fail;
        if (Lbit(insn)) {
            // ldr
            if (Bbit(insn))
                val = *(uint8*)newaddr;
            else
                val = *(uint32*)newaddr;
            setReg(regs, mask_Rd(insn), val);
        } else {
            // str
            val = getReg(regs, mask_Rd(insn));
            if (Bbit(insn))
                *(uint8*)newaddr = val;
            else
                *(uint32*)newaddr = val;
        }
        if (Wbit(insn))
            setReg(regs, mask_Rn(insn), addr);
    } else if ((insn & 0x0E000090) == 0x00000090) {
        if (Pbit(insn) == 0)
            goto fail;
        if (Lbit(insn)) {
            // ldrh
            if (Hbit(insn)) {
                if (Sbit(insn)) {
                    val = *(int16*)newaddr;
                } else {
                    val = *(uint16*)newaddr;
                }
            } else if (Sbit(insn)) {
                val = *(int8*)newaddr;
            } else
                goto fail;
            setReg(regs, mask_Rd(insn), val);
        } else {
            // strh
            if (Sbit(insn) || !Hbit(insn))
                goto fail;
            val = getReg(regs, mask_Rd(insn));
            *(uint16*)newaddr = val;
        }
        if (Wbit(insn))
            setReg(regs, mask_Rn(insn), addr);
    } else
        goto fail;
    add_trace(data, report_memAccess, addr, old_pc, insn, val);
    return;
fail:
    add_trace(data, report_memAccess, addr, old_pc, insn
              , getReg(regs, mask_Rd(insn))
              , getReg(regs, mask_Rn(insn)));
    giveUp(data, regs, addr);
}

// Obtain the Fault Address Register.
DEF_GETIRQCPR(get_FAR, p15, 0, c6, c0, 0)

// Check if two addresses are in the same 1Meg range.
static inline int __irq addrmatch(uint32 addr1, uint32 addr2) {
    return ((addr1 ^ addr2) & TOPBITS) == 0;
}

int __irq
L1_abort_handler(struct irqData *data, struct irqregs *regs)
{
    if (!data->l1traceDesc)
        return 0;
    uint32 faultaddr = get_FAR();
    if (!addrmatch(faultaddr, data->l1trace))
        // Not a fault to memory being traced
        return 0;
    tryEmulate(data, regs, faultaddr);
    return 1;
}

int __irq
L1_prefetch_handler(struct irqData *data, struct irqregs *regs)
{
    // XXX
    return 0;

    if (!data->l1traceDesc)
        return 0;
    uint32 faultaddr = get_FAR();
    if (!addrmatch(faultaddr, data->l1trace))
        // Not a fault to memory being traced
        return 0;
    giveUp(data, regs, faultaddr);
    return 1;
}

static uint32 L1Trace = 0xFFFFFFFF;
REG_VAR_INT(testWirqAvail, "L1TRACE", L1Trace,
            "1Meg Memory location to trace during WI")
static uint32 AltL1Trace = 0xE1100000;
REG_VAR_INT(testWirqAvail, "ALTL1TRACE", AltL1Trace,
            "Alternate location to move L1TRACE accesses to")
static uint32 MaxL1Trace = 0xFFFFFFFF;
REG_VAR_INT(testWirqAvail, "MAXL1TRACE", MaxL1Trace,
            "Maximum number of l1traces to report before giving up (debug feature)")

static uint32 *
getMMUref(uint32 vaddr)
{
    return (uint32*)memPhysMap(cpuGetMMU() + ((MVAddr(vaddr) >> 18) & ~3));
}

static void
alterTracePoint(struct irqData *data, memcheck *mc)
{
    uint32 addr = (uint32)mc->addr;
    if (!addrmatch(data->l1trace, addr))
        return;
    mc->addr = (char *)data->alt_l1trace + (addr & BOTBITS);
}

int
prepL1traps(struct irqData *data)
{
    if (L1Trace == 0xFFFFFFFF)
        // Nothing to do.
        return 0;
    if (L1Trace & BOTBITS) {
        Output("L1Trace=%08x is not 1Meg aligned", L1Trace);
        return -1;
    }
    if (AltL1Trace & BOTBITS) {
        Output("AltL1Trace=%08x is not 1Meg aligned", AltL1Trace);
        return -1;
    }

    // Lookup descriptor for mappings
    uint32 *l1d = getMMUref(L1Trace);
    if ((*l1d & MMU_L1_TYPE_MASK) != MMU_L1_SECTION) {
        Output("Address %08x not an L1 mapping (l1d=%08x)"
               , L1Trace, *l1d);
        return -1;
    }
    uint32 *alt_l1d = getMMUref(AltL1Trace);
    if (*alt_l1d != MMU_L1_UNMAPPED) {
        Output("Address %08x not unmapped (l1d=%08x)"
               , AltL1Trace, *alt_l1d);
        return -1;
    }

    data->l1trace = L1Trace;
    data->l1traceDesc = l1d;
    data->alt_l1trace = AltL1Trace;
    data->alt_l1traceDesc = alt_l1d;
    data->max_l1trace = MaxL1Trace;

    Output("Mapping %08x accesses to %08x (tbl %p=%08x/%p=%08x)"
           , data->l1trace, data->alt_l1trace
           , data->l1traceDesc, *data->l1traceDesc
           , data->alt_l1traceDesc, *data->alt_l1traceDesc);

    // Redirect any traces of target area to alt area.
    for (uint i=0; i<data->tracepollcount; i++)
        alterTracePoint(data, &data->tracepolls[i]);
    for (uint i=0; i<data->irqpollcount; i++)
        alterTracePoint(data, &data->irqpolls[i]);

    return 0;
}
