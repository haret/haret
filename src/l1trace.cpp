/* Alter MMU table to catch reads/writes to memory.
 *
 * (C) Copyright 2007 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <string.h> // memset

#include "script.h" // REG_VAR_INT
#include "output.h" // Output
#include "memory.h" // MMU_L1_UNMAPPED
#include "arminsns.h" // getReg
#include "irq.h"

#define ONEMEG (1024*1024)
#define TOPBITS 0xFFF00000
#define BOTBITS 0x000FFFFF

/*
 * Theory of operation:
 *
 * This code allows one to obtain a log of reads and writes to memory
 * that wince (and programs running under wince) perform.  This is
 * useful for "watching" how the system interacts with memory mapped
 * hardware registers.
 *
 * The code works by altering the ARM cpu memory management unit (MMU)
 * tables so that ranges of virtual addresses cause a memory access
 * fault.  In this way, haret can cause all code on the system that
 * reads or writes to a memory location to fault.
 *
 * Haret then binds the memory exception vectors so that haret can
 * analyze each fault.  Normal access faults are handed off to the
 * regular wince exception handler unmodified.  However, faults that
 * are caused by the altering of the MMU are handled by haret.  Each
 * generated fault causes haret to emulate the access, log the access
 * for the user, and then return to the original code.
 *
 * This system should be portable to all ARM cpus capable of running
 * wince.
 */


/****************************************************************
 * MMU fault handling
 ****************************************************************/

// Return a pointer to an MMU descriptor for a given vaddr
static inline uint32 * __irq
getMMUref(struct irqData *data, uint32 vaddr) {
    return data->mmuVAddr + (MVAddr_irq(vaddr) >> 20);
}

// Return the redirected address for the nth masked addr
static inline uint32 __irq
newAddr(struct irqData *data, uint i) {
    return data->redirectVAddrBase + ONEMEG * i;
}

// Alter MMU table so that certain memory accesses will fault.
void
startL1traps(struct irqData *data)
{
    if (!data->alterCount)
        return;
    // Unmap the l1 mmu reference
    for (uint i=0; i<data->alterCount; i++) {
        uint32 *desc = getMMUref(data, data->alterVAddrs[i]);
        uint32 *redirectdesc = getMMUref(data, newAddr(data, i));
        if ((*desc & MMU_L1_TYPE_MASK) == MMU_L1_SECTION) {
            // XXX - set domain
            data->alterVAddrs[i] |= *desc & BOTBITS;
            *redirectdesc = ((*desc | MMU_L1_AP_MASK)
                             & ~(MMU_L1_CACHEABLE | MMU_L1_BUFFERABLE));
        } else {
            *redirectdesc = *desc;
        }
        *desc = MMU_L1_UNMAPPED;
    }
}

static void
report_mapchanged(irqData *, const char *header, traceitem *item)
{
    uint32 mmuaddr=item->d0, val=item->d1;
    Output("%s ERROR! Mapping at %08x changed from %08x to %08x"
           , header, mmuaddr, MMU_L1_UNMAPPED, val);
}

// Return MMU table to its original state
void __irq
stopL1traps(struct irqData *data)
{
    if (!data->alterCount)
        return;
    // Restore the previous l1 mmu info.
    for (uint i=0; i<data->alterCount; i++) {
        uint32 *desc = getMMUref(data, data->alterVAddrs[i]);
        uint32 *redirectdesc = getMMUref(data, newAddr(data, i));
        if (*desc != MMU_L1_UNMAPPED)
            // Something messed with the mapping?
            add_trace(data, report_mapchanged, (uint32)desc, *desc);
        if ((*redirectdesc & MMU_L1_TYPE_MASK) == MMU_L1_SECTION)
            *desc = (*redirectdesc & TOPBITS) | (data->alterVAddrs[i] & BOTBITS);
        else
            *desc = *redirectdesc;
        *redirectdesc = MMU_L1_UNMAPPED;
    }
    data->alterCount = 0;
}

// Obtain the Fault Address Register.
DEF_GETIRQCPR(get_FAR, p15, 0, c6, c0, 0)
// Get the Fault Status Register.
DEF_GETIRQCPR(get_FSR, p15, 0, c5, c0, 0)
// Flush the I and D TLBs.
DEF_SETCPRATTR(set_TLBflush, p15, 0, c8, c7, 0, __irq, "memory")

static void
report_giveup(irqData *, const char *header, traceitem *)
{
    Output("%s giving up - clearing mapping", header);
}

// A problem occurred during emulation - try to report the problem and
// turn off future traps.
static void __irq
giveUp(struct irqData *data)
{
    add_trace(data, report_giveup);

    data->errors++;
    data->tracepoll.count = 0;
    data->irqpoll.count = 0;
    data->exitEarly = 1;

    stopL1traps(data);
    set_TLBflush(0);
}

static void
report_memAccess(irqData *, const char *header, traceitem *item)
{
    uint32 addr=item->d0, pc=item->d1, insn=item->d2, val=item->d3;
    uint32 changed=item->d4;
    Output("%s mmutrace %08x: %08x(%s) %08x %08x (%08x)"
           , header, pc, insn, getInsnName(insn), addr, val, changed);
}

// Event reporting
// Check if we're interested in this operation and if so record it.
static void __irq
reportAddr(struct irqData *data, uint32 addr, uint32 val,
           uint32 old_pc, uint32 insn, uint32 addrsize)
{
    if (isIgnoredAddr(data, old_pc))
        // Not interested in this address
        return;

    for (uint i=0; i<data->traceCount; i++) {
        memcheck *t = &data->traceAddrs[i];
        if (! RANGES_OVERLAP(addr, addrsize, t->addr, t->rangesize))
            continue;
        if (Lbit(insn)) {
            if (!(t->rw & 1))
                // Not interested in reads
                continue;
        } else {
            if (!(t->rw & 2))
                // Not interested in writes
                continue;
        }

        uint32 shift = 8 * (addr & 3);
        uint32 mask = ((1 << (8 * addrsize)) - 1) << shift;
        uint32 cmpval = ((val << shift) & mask) | (t->cmpVal & ~mask);
        uint32 changed;
        if (testChanged(t, cmpval, &changed))
            // Report insn
            add_trace(data, report_memAccess, addr, old_pc, insn
                      , val, changed >> shift);
        break;
    }
}

// Attempt to emulate a memory access fault, and then report the event
// to the user.
static void __irq
tryEmulate(struct irqData *data, struct irqregs *regs
           , uint32 addr, uint32 newaddr)
{
    uint32 old_pc = MVAddr_irq(regs->old_pc - 8);
    uint32 insn = 0;
    if (get_SPSR() & (1<<5))
        // Don't know how to handle thumb mode.
        goto fail;
    insn = *(uint32*)old_pc;
    if ((get_FSR() & 0xd) != 0x5)
        // Not a "section translation fault".
        goto fail;

    if (--data->max_l1trace == 0)
        goto fail;

    // Emulate instrution
    uint32 addrsize;
    uint32 val;
    if ((insn & 0x0Fb000F0) == 0x01000090) {
        // Swap instruction
        uint32 readval, writeval = getReg(regs, mask_Rm(insn));
        if (Bbit(insn)) {
            addrsize = 1;
            asm("swpb %0, %1, [%2]"
                : "=r" (readval)
                : "r" (writeval), "r" (newaddr));
        } else {
            addrsize = 4;
            asm("swp %0, %1, [%2]"
                : "=r" (readval)
                : "r" (writeval), "r" (newaddr));
        }
        setReg(regs, mask_Rd(insn), readval);
        // report swap
        reportAddr(data, addr, readval, old_pc, insn, addrsize);
        reportAddr(data, addr, writeval, old_pc, insn, addrsize);
    } else if ((insn & 0x0C000000) == 0x04000000) {
        // load/store single single
        if (!Pbit(insn) && (Wbit(insn) || Ibit(insn)))
            // insn not supported
            goto fail;
        addrsize = Bbit(insn) ? 1 : 4;
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

        if (Pbit(insn) == 0) {
            // post index
            uint32 offset = insn & 0xFFF;
            uint32 Rn = addr + offset;
            if (!Ubit(insn))
                // subtracting
                Rn = addr - offset;
            setReg(regs, mask_Rn(insn), Rn);
        } else if (Wbit(insn))
            // pre-index and write back
            // XXX - addr is an MVA - but reg might be a VA.
            setReg(regs, mask_Rn(insn), addr);

        // report single op
        reportAddr(data,addr,val,old_pc,insn,addrsize);
    } else if ((insn & 0x0E000090) == 0x00000090) {
        // halfword transfer single
        if (!Pbit(insn) && (Wbit(insn) || !Bbit(insn)))
            goto fail;
        addrsize = 2;
        if (Lbit(insn)) {
            if (Hbit(insn)) {
                if (Sbit(insn))
                    // ldrsh
                    val = *(int16*)newaddr;
                else
                    // ldrh
                    val = *(uint16*)newaddr;
            } else if (Sbit(insn)) {
                // ldrsb
                addrsize = 1;
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
        if (Pbit(insn) == 0) {
            uint32 offset = ((insn & 0xF00) >> 4) | (insn & 0xF);
            uint32 Rn = addr + offset;
            if (!Ubit(insn))
                Rn = addr - offset;
            setReg(regs, mask_Rn(insn), Rn);
        } else if (Wbit(insn))
            setReg(regs, mask_Rn(insn), addr);

        // report single op
        reportAddr(data,addr,val,old_pc,insn,addrsize);
    } else if ((insn & 0x0E000000) == 0x08000000) {
        // multi word transfer
        if (Bbit(insn))
            // this is 'S' = "PSR" bit - not supported
            goto fail;

        if ((insn & 0x0000FFFF) == 0)
            // no registers being copied = invalid instruction
            goto fail;

        val = 0;
        addrsize = 4; //suppress warnings
        int regCount = 0;
        // It is assumed that the first read/write caused the fault
        // (If its possible that its not, this might break.)

        // It is the lowest memory address which causes the fault and
        // whether we are incrementing or decrementing, the lowest reg
        // always goes in the lowest memory address.
        for (int r=0; r<16; r++){
            //for each register, upwards
            if (! (insn & (1 << r)))
                // register not being copied
                continue;

            if (Lbit(insn)) {
                // load
                val = *(uint32*)newaddr;
                setReg(regs, r, val);
            } else {
                // store
                val = getReg(regs, r);
                *(uint32*)newaddr = val;
            }
            // report every read/write as seperate access
            reportAddr(data, addr, val, old_pc, insn, addrsize);

            addr += 4;
            newaddr += 4;
            regCount++;
        }
        // because of the different forms (pre-inc, post-dec etc),
        // addr and newaddr may or may not be in the right place now,
        // but we don't care

        // if write-back, do so
        if (Wbit(insn)) {
            // regardless of pre/post indexing, base reg always ends
            // up regCount from where it started

            // fetch the address in the base register
            uint32 baseReg = getReg(regs, mask_Rn(insn));
            if (Ubit(insn))
                // incrementing
                baseReg += 4 * regCount;
            else
                // decrementing
                baseReg -= 4 * regCount;
            setReg(regs, mask_Rn(insn), baseReg);
        }
    } else
        goto fail;

    return;

fail:
    add_trace(data, report_memAccess, addr, old_pc, insn
              , getReg(regs, mask_Rd(insn))
              , getReg(regs, mask_Rn(insn)));
    // Rerun instruction on return to user code.
    regs->old_pc -= 4;
    giveUp(data);
}

// Check if two addresses are in the same 1Meg range.
static inline int __irq addrmatch(uint32 addr1, uint32 addr2) {
    return ((addr1 ^ addr2) & TOPBITS) == 0;
}

// Called during memory access faults - it determines if the fault was
// caused by our MMU table manipulations.
int __irq
L1_abort_handler(struct irqData *data, struct irqregs *regs)
{
    if (!data->alterCount)
        return 0;
    uint32 faultaddr = get_FAR();
    for (uint i=0; i<data->alterCount; i++)
        if (addrmatch(faultaddr, data->alterVAddrs[i])) {
            // Fault to memory being traced
            uint32 newaddr = newAddr(data, i) | (faultaddr & BOTBITS);
            tryEmulate(data, regs, faultaddr, newaddr);
            return 1;
        }
    return 0;
}

static void
report_prefetch(irqData *, const char *header, traceitem *item)
{
    uint32 addr=item->d0;
    Output("%s Can't emulate insn access at %08x", header, addr);
}

// Handler for instruction fetch faults - this is here to catch the
// unlikely event that code jumps to an area of memory that is being
// watched.
int __irq
L1_prefetch_handler(struct irqData *data, struct irqregs *regs)
{
    uint32 faultaddr = MVAddr_irq(regs->old_pc - 4);
    for (uint i=0; i<data->alterCount; i++)
        if (addrmatch(faultaddr, data->alterVAddrs[i])) {
            add_trace(data, report_prefetch, faultaddr);
            giveUp(data);
            return 1;
        }
    return 0;
}

// Handler for wince resume
void __irq
L1_resume_handler(struct irqData *data, struct irqregs *regs)
{
    if (data->max_l1trace_after_resume)
        data->max_l1trace = data->max_l1trace_after_resume;
}


/****************************************************************
 * Tracing variables
 ****************************************************************/

static uint32 AltL1Trace = 0xE1100000;
REG_VAR_INT(0, "ALTL1TRACE", AltL1Trace,
            "Alternate location to move L1TRACE accesses to")
static uint32 MaxL1Trace = 0;
REG_VAR_INT(0, "MAXL1TRACE", MaxL1Trace,
            "Maximum number of l1traces to report before giving up (debug feature)")
static uint32 MaxL1TraceAfterResume = 0;
REG_VAR_INT(0, "MAXL1TRACERESUME", MaxL1TraceAfterResume,
            "Maximum number of l1traces after resume (debug feature)")

class traceListVar : public listVarBase {
public:
    uint32 tracecount;
    memcheck traces[MAX_L1TRACE];
    traceListVar(predFunc ta, const char *n, const char *d)
        : listVarBase("var_list_trace", ta, n, d
                      , &tracecount, (void*)traces, sizeof(traces[0])
                      , ARRAY_SIZE(traces))
        , tracecount(0) { }
    variableBase *newVar() { return new traceListVar(0, "", ""); }
    bool setVarItem(void *p, const char *args) {
        memcheck *t = (memcheck*)p;
        memset(t, 0, sizeof(*t));
        if (!get_expression(&args, &t->addr)) {
            ScriptError("Expected <start>");
            return false;
        }
        const char *flags = "rw";
        char _flags[16];
        uint32 rangesize = 1;
        uint32 mask = 0;
        if (get_expression(&args, &rangesize))
            if (!get_token(&args, _flags, sizeof(_flags))) {
                flags = _flags;
                if (get_expression(&args, &mask)) {
                    t->trySuppressNext = 1;
                    t->setCmp = 1;
                    get_suppress(args, t);
                }
            }

        // Adjust mask to address offset
        uint32 shift = 8 * (t->addr & 3);
        t->mask = rotl(~mask, shift);
        t->cmpVal = rotl(t->cmpVal, shift);
        t->rangesize = rangesize;
        t->rw = 0;
        if (strchr(flags, 'r') || strchr(flags, 'R'))
            t->rw = 1;
        if (strchr(flags, 'w') || strchr(flags, 'W'))
            t->rw |= 2;
        return true;
    }
    void showVar(const char *args) {
        for (uint i=0; i<tracecount; i++) {
            memcheck *t = &traces[i];
            char flags[3], *f = flags;
            if (t->rw & 1)
                *f++ = 'r';
            if (t->rw & 2)
                *f++ = 'w';
            *f = '\0';
            char buf[32];

            // Undo adjustment of mask to address offset
            memcheck tmp = *t;
            uint32 shift = 8 * (t->addr & 3);
            t->cmpVal = rotr(t->cmpVal, shift);

            Output("%03d: 0x%08x %d '%s' %08x %s"
                   , i, t->addr, t->rangesize, flags
                   , rotr(~t->mask, shift), disp_suppress(&tmp, buf));
        }
    }
};
__REG_VAR(
    traceListVar, MMUTrace, 0,
    "MMUTRACE",
    "Memory locations to trace during WIRQ.\n"
    "  List of <start> [<size> [<rw> [<mask> [<ignVal>]]]] 5-tuples\n"
    "    <start>  is a virtual address to trace\n"
    "    <size>   is the number of bytes in the range to trace (default 1)\n"
    "    <rw>     is a string (eg, 'r') that determines if reads and/or\n"
    "             writes are reported (default 'rw')\n"
    "    <mask>   is a bitmask to ignore when detecting a change (default 0)\n"
    "    <ignVal> report only when value doesn't equal this value - one\n"
    "             may also specify 'last' or 'none' (default is 'last' if\n"
    "             <mask> is set - report on change.  Otherwise the default\n"
    "             is 'none' - always report every event)")

static uint32 ignoreAddr[MAX_IGNOREADDR];
static uint32 ignoreAddrCount;
REG_VAR_INTLIST(0, "TRACEIGNORE", &ignoreAddrCount, ignoreAddr,
                "List of pc addresses to ignore when tracing")

// When tracing a coarse (or fine) mapping in the page tables there is
// no reliable way to disable caching.  So, if an access occurs that
// can't be emulated and the tracing needs to stop early then any
// cached memory wont be flushed - this could cause problems.  By
// default we only allow tracing section mappings - but we allow the
// user to override the sanity test because there are occasionally
// valid uses.
static uint32 PermissiveMMUtrace;
REG_VAR_INT(0, "PERMISSIVEMMUTRACE", PermissiveMMUtrace,
            "Permit tracing non-section memory addresses.")


/****************************************************************
 * Tracing setup
 ****************************************************************/

// Manipulate IRQ time memory polling structures so that the memory
// polls don't access the memory areas that are being made to fault.
static void
alterTracePoint(struct irqData *data, memcheck *mc)
{
    for (uint i=0; i<data->alterCount; i++)
        if (addrmatch(data->alterVAddrs[i], mc->addr))
            mc->addr = newAddr(data, i) | (mc->addr & BOTBITS);
}

// Prepare for memory tracing.
int
prepL1traps(struct irqData *data)
{
    // Copy ignoreaddr values (note pxatrace uses these too)
    data->ignoreAddrCount = ignoreAddrCount;
    memcpy(data->ignoreAddr, ignoreAddr, sizeof(data->ignoreAddr));

    data->traceCount = MMUTrace.tracecount;
    if (!data->traceCount)
        // Nothing to do.
        return 0;
    memcpy(data->traceAddrs, MMUTrace.traces, sizeof(data->traceAddrs));

    data->redirectVAddrBase = AltL1Trace;
    if (data->redirectVAddrBase & BOTBITS) {
        Output("AltL1Trace=%08x is not 1Meg aligned", data->redirectVAddrBase);
        return -1;
    }

    data->mmuVAddr = (uint32*)memPhysMap(cpuGetMMU());
    if (! data->mmuVAddr) {
        Output("Unable to map MMU table");
        return -1;
    }

    data->max_l1trace = MaxL1Trace;
    data->max_l1trace_after_resume = MaxL1TraceAfterResume;

    // Determine which 1Meg sections need to be altered
    uint32 count = 0;
    for (uint i=0; i<data->traceCount; i++) {
        memcheck *t = &data->traceAddrs[i];
        for (uint32 vaddr = t->addr & TOPBITS
                 ; RANGES_OVERLAP(vaddr, ONEMEG, t->addr, t->rangesize)
                 ; vaddr += ONEMEG) {
            // See if this vaddr already done.
            int found = 0;
            for (uint j=0; j<count; j++)
                if (data->alterVAddrs[j] == vaddr)
                    found = 1;
            if (found)
                continue;

            if (count >= MAX_L1TRACE) {
                Output("Too many L1 traces (max=%d)", MAX_L1TRACE);
                return -1;
            }

            // Lookup descriptor for mappings
            uint32 l1d = *getMMUref(data, vaddr);
            if ((l1d & MMU_L1_TYPE_MASK) != MMU_L1_SECTION) {
                Output("Warning! Tracing non-section mapping (%08x)"
                       " not well supported", vaddr);
                if (!PermissiveMMUtrace) {
                    Output("If you really want to do this, run"
                           " 'set permissivemmutrace 1' and retry");
                    return -1;
                }
            }
            uint32 alt_l1d = *getMMUref(data, newAddr(data, count));
            if (alt_l1d != MMU_L1_UNMAPPED) {
                Output("Address %08x not unmapped (l1d=%08x)"
                       , newAddr(data, count), alt_l1d);
                return -1;
            }

            data->alterVAddrs[count] = vaddr;
            Output("%02d: Mapping %08x(@%08x) accesses to %08x (tbl %08x)"
                   , count, vaddr, memVirtToPhys(vaddr), newAddr(data, count)
                   , l1d);
            count++;
        }
    }
    data->alterCount = count;

    // Redirect any traces of target area to alt area.
    for (uint i=0; i<data->tracepoll.count; i++)
        alterTracePoint(data, &data->tracepoll.list[i]);
    for (uint i=0; i<data->irqpoll.count; i++)
        alterTracePoint(data, &data->irqpoll.list[i]);

    return 0;
}
