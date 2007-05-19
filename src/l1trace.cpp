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

void __irq
stopL1traps(struct irqData *data)
{
    if (!data->alterCount)
        return;
    // Restore the previous l1 mmu info.
    for (uint i=0; i<data->alterCount; i++) {
        uint32 *desc = getMMUref(data, data->alterVAddrs[i]);
        uint32 *redirectdesc = getMMUref(data, newAddr(data, i));
        if ((*redirectdesc & MMU_L1_TYPE_MASK) == MMU_L1_SECTION)
            *desc = (*redirectdesc & TOPBITS) | (data->alterVAddrs[i] & BOTBITS);
        else
            *desc = *redirectdesc;
        *redirectdesc = MMU_L1_UNMAPPED;
    }
    data->alterCount = 0;
}

// Flush the I and D TLBs.
DEF_SETCPRATTR(set_TLBflush, p15, 0, c8, c7, 0, __irq, "memory")

static void
report_giveup(uint32 msecs, irqData *, traceitem *item)
{
    Output("%06d: %08x: giving up - clearing mapping"
           , msecs, 0);
}

static void __irq
giveUp(struct irqData *data)
{
    add_trace(data, report_giveup);
    data->errors++;
    stopL1traps(data);
    set_TLBflush(0);
}

static void
report_memAccess(uint32 msecs, irqData *, traceitem *item)
{
    uint32 addr=item->d0, pc=item->d1, insn=item->d2, val=item->d3;
    Output("%06d: debug %08x: %08x(%s) %08x %08x"
           , msecs, pc, insn, getInsnName(insn), val, addr);
}

static void __irq
tryEmulate(struct irqData *data, struct irqregs *regs
           , uint32 addr, uint32 newaddr)
{
    uint32 old_pc = MVAddr_irq(regs->old_pc - 8);
    uint32 insn = *(uint32*)old_pc;

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

    // Emulate instrution
    int addrsize;
    uint32 val;
    if ((insn & 0x0C000000) == 0x04000000) {
        if (!Pbit(insn) && (Wbit(insn) || Ibit(insn)))
            goto fail;
        addrsize = Bbit(insn) ? 1 : 4;
        if (Lbit(insn)) {
            // ldr
            if (Bbit(insn))
                // XXX - access could fault
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
            uint32 offset = insn & 0xFFF;
            uint32 Rn = addr + offset;
            if (!Ubit(insn))
                Rn = addr - offset;
            setReg(regs, mask_Rn(insn), Rn);
        } else if (Wbit(insn))
            // XXX - addr is an MVA - but reg might be a VA.
            setReg(regs, mask_Rn(insn), addr);
    } else if ((insn & 0x0E000090) == 0x00000090) {
        if (!Pbit(insn) && (Wbit(insn) || !Bbit(insn)))
            goto fail;
        addrsize = 2;
        if (Lbit(insn)) {
            // ldrh
            if (Hbit(insn)) {
                if (Sbit(insn)) {
                    val = *(int16*)newaddr;
                } else {
                    val = *(uint16*)newaddr;
                }
            } else if (Sbit(insn)) {
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
    } else
        goto fail;

    // See if this instruction should be reported
    if (isIgnoredAddr(data, old_pc))
        return;
    for (uint i=0; i<data->traceCount; i++) {
        irqData::trace_s *t = &data->traceAddrs[i];
        if (t->start >= addr + addrsize || t->end <= addr)
            continue;
        if (Lbit(insn)) {
            if (!(t->rw & 1))
                break;
        } else {
            if (!(t->rw & 2))
                break;
        }
        // Report insn
        add_trace(data, report_memAccess, addr, old_pc, insn, val);
        break;
    }
    return;
fail:
    add_trace(data, report_memAccess, addr, old_pc, insn
              , getReg(regs, mask_Rd(insn))
              , getReg(regs, mask_Rn(insn)));
    // Rerun instruction on return to user code.
    regs->old_pc -= 4;
    giveUp(data);
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
report_prefetch(uint32 msecs, irqData *, traceitem *item)
{
    uint32 addr=item->d0;
    Output("%06d: %08x: Can't emulate insn access at %08x"
           , msecs, 0, addr);
}

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


/****************************************************************
 * Tracing variables
 ****************************************************************/

static uint32 AltL1Trace = 0xE1100000;
REG_VAR_INT(testWirqAvail, "ALTL1TRACE", AltL1Trace,
            "Alternate location to move L1TRACE accesses to")
static uint32 MaxL1Trace = 0xFFFFFFFF;
REG_VAR_INT(testWirqAvail, "MAXL1TRACE", MaxL1Trace,
            "Maximum number of l1traces to report before giving up (debug feature)")

class traceListVar : public listVarBase {
public:
    uint32 tracecount;
    irqData::trace_s traces[MAX_L1TRACE];
    traceListVar(predFunc ta, const char *n, const char *d)
        : listVarBase(ta, n, d
                      , &tracecount, (void*)traces, sizeof(traces[0])
                      , ARRAY_SIZE(traces)) { }
    bool setVarItem(void *p, const char *args) {
        irqData::trace_s *t = (irqData::trace_s*)p;
        if (!get_expression(&args, &t->start)) {
            ScriptError("Expected <start>");
            return false;
        }
        const char *flags = "rw";
        char _flags[16];
        uint32 addrsize = 1;
        if (get_expression(&args, &addrsize))
            if (!get_token(&args, _flags, sizeof(_flags)))
                flags = _flags;
        t->end = t->start + addrsize;
        if ((t->start & TOPBITS) != ((t->end - 1) & TOPBITS)) {
            ScriptError("Address range must be within a 1Meg section");
            return false;
        }
        t->rw = 0;
        if (strchr(flags, 'r') || strchr(flags, 'R'))
            t->rw = 1;
        if (strchr(flags, 'w') || strchr(flags, 'W'))
            t->rw |= 2;
        return true;
    }
    void showVar(const char *args) {
        for (uint i=0; i<tracecount; i++) {
            irqData::trace_s *t = &traces[i];
            char flags[3], *f = flags;
            if (t->rw & 1)
                *f++ = 'r';
            if (t->rw & 2)
                *f++ = 'w';
            *f = '\0';
            Output("%03d: 0x%08x %d %s", i, t->start, t->end - t->start, flags);
        }
    }
    void fillVarType(char *buf) {
        strcpy(buf, "trace list");
    }
};
__REG_VAR(traceListVar, MMUTrace,
          testWirqAvail,
          "MMUTRACE",
          "Memory locations to trace during WI.\n"
          "  List of <start> [<size> [<rw>]] triples where <start> is a\n"
          "  virtual address to trace, <size> is the number of bytes in the\n"
          "  range to trace (default 1), and <rw> is a string (eg, 'r') that\n"
          "  determines if reads and/or writes are reported (default 'rw').")

static uint32 ignoreAddr[MAX_IGNOREADDR];
static uint32 ignoreAddrCount;
REG_VAR_INTLIST(testWirqAvail, "TRACEIGNORE", &ignoreAddrCount, ignoreAddr,
                "List of pc addresses to ignore when tracing")


/****************************************************************
 * Tracing setup
 ****************************************************************/

static void
alterTracePoint(struct irqData *data, memcheck *mc)
{
    uint32 addr = (uint32)mc->addr;
    for (uint i=0; i<data->alterCount; i++)
        if (addrmatch(data->alterVAddrs[i], addr))
            mc->addr = (char *)(newAddr(data, i) | (addr & BOTBITS));
}

int
prepL1traps(struct irqData *data)
{
    // Copy ignoreaddr values (note pxatrace uses this too)
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

    // Determine which 1Meg sections need to be altered
    uint32 count = 0;
    for (uint i=0; i<data->traceCount; i++) {
        irqData::trace_s *t = &data->traceAddrs[i];
        uint32 vaddr = t->start & TOPBITS;

        // See if this vaddr already done.
        int found = 0;
        for (uint j=0; j<count; j++)
            if (data->alterVAddrs[j] == vaddr)
                found = 1;
        if (found)
            continue;

        // Lookup descriptor for mappings
        uint32 l1d = *getMMUref(data, vaddr);
        if ((l1d & MMU_L1_TYPE_MASK) != MMU_L1_SECTION)
            // When tracing a coarse (or fine) mapping in the page
            // tables there is no reliable way to disable caching.
            // So, if an access occurs that can't be emulated and the
            // tracing needs to stop early then any cached memory wont
            // be flushed - this could cause problems.
            Output("Warning! Tracing non-section mapping (%08x)"
                   " not well supported", vaddr);
        uint32 alt_l1d = *getMMUref(data, newAddr(data, count));
        if (alt_l1d != MMU_L1_UNMAPPED) {
            Output("Address %08x not unmapped (l1d=%08x)"
                   , newAddr(data, count), alt_l1d);
            return -1;
        }

        data->alterVAddrs[count] = vaddr;
        Output("%02d: Mapping %08x accesses to %08x (tbl %08x)"
               , i, data->alterVAddrs[count], newAddr(data, count), l1d);
        count++;
    }
    data->alterCount = count;

    // Redirect any traces of target area to alt area.
    for (uint i=0; i<data->tracepollcount; i++)
        alterTracePoint(data, &data->tracepolls[i]);
    for (uint i=0; i<data->irqpollcount; i++)
        alterTracePoint(data, &data->irqpolls[i]);

    return 0;
}
