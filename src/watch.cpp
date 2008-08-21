/* Poll areas of memory for changes.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <windows.h> // Sleep
#include "pkfuncs.h" // SleepTillTick
#include <ctype.h> // toupper
#include <stdio.h> // _snprintf

#include "output.h" // Output
#include "arminsns.h" // runArmInsn
#include "lateload.h" // LATE_LOAD
#include "script.h" // REG_CMD
#include "irq.h" // __irq
#include "memory.h" // memVirtToPhys
#include "watch.h"

// Older versions of wince don't have SleepTillTick - use Sleep(1)
// instead.
static void
alt_SleepTillTick()
{
    Sleep(1);
}
LATE_LOAD_ALT(SleepTillTick, "coredll")

static inline uint32 __irq
doRead(struct memcheck *mc)
{
    if (mc->isInsn)
        return runArmInsn(mc->insn, 0);
    switch (mc->readSize) {
    default:
    case MO_SIZE32:
        return *(uint32*)mc->addr;
    case MO_SIZE16:
        return *(uint16*)mc->addr;
    case MO_SIZE8:
        return *(uint8*)mc->addr;
    }
}

int __irq
testChanged(struct memcheck *mc, uint32 curval, uint32 *pchanged)
{
    uint32 changed = 0;
    if (mc->trySuppress) {
        changed = (curval ^ mc->cmpVal) & mc->mask;
        if (changed == 0)
            // No change in value.
            return 0;
    }
    if (mc->setCmp)
        mc->cmpVal = curval;
    mc->trySuppress = mc->trySuppressNext;
    *pchanged = changed;
    return 1;
}

// Read a memory area and check for a change.
int __irq
testMem(struct memcheck *mc, uint32 *pnewval, uint32 *pchanged)
{
    uint32 curval = doRead(mc);
    if (testChanged(mc, curval, pchanged)) {
        *pnewval = curval;
        return 1;
    }
    return 0;
}


/****************************************************************
 * Helpers for registering watched memory
 ****************************************************************/

void
get_suppress(const char *args, memcheck *mc)
{
    char nexttoken[16];
    const char *nextargs = args;
    if (get_token(&nextargs, nexttoken, sizeof(nexttoken)))
        return;
    if (_stricmp(nexttoken, "last") == 0) {
        mc->trySuppressNext = 1;
        mc->setCmp = 1;
    } else if (_stricmp(nexttoken, "none") == 0) {
        mc->trySuppressNext = 0;
        mc->setCmp = 0;
    } else if (get_expression(&args, &mc->cmpVal)) {
        mc->trySuppressNext = 1;
        mc->setCmp = 0;
    }
}

const char *
disp_suppress(memcheck *mc, char *buf)
{
    if (!mc->trySuppressNext)
        return "none";
    if (mc->setCmp)
        return "last";
    sprintf(buf, "%d", mc->cmpVal);
    return buf;
}

watchListVar *watchListVar::cast(commandBase *b) {
    if (b && b->isAvail && strncmp(b->type, "var_list_watch", 14) == 0)
        return static_cast<watchListVar*>(b);
    return NULL;
}

bool
watchListVar::setVarItem(void *p, const char *args)
{
    // Parse args
    uint32 mask = 0;
    memcheck *mc = (memcheck*)p;
    memset(mc, 0, sizeof(*mc));
    mc->setCmp = 1;
    mc->trySuppressNext = 1;

    char nexttoken[16];
    const char *nextargs = args;
    if (get_token(&nextargs, nexttoken, sizeof(nexttoken))) {
        ScriptError("Expected <addr> or CP or CPSR or SPSR");
        return false;
    }
    if (_stricmp(nexttoken, "CP") == 0) {
        // CP watch.
        uint cp, op1, CRn, CRm, op2;
        args = nextargs;
        if (!get_expression(&args, &cp) || !get_expression(&args, &op1)
            || !get_expression(&args, &CRn) || !get_expression(&args, &CRm)
            || !get_expression(&args, &op2)) {
            ScriptError("Expected CP <cp#> <op1> <CRn> <CRm> <op2>");
            return false;
        }
        mc->addr = buildArmCPInsn(0, cp, op1, CRn, CRm, op2);
        mc->isInsn = 1;
        if (get_expression(&args, &mask))
            get_suppress(args, mc);
        mc->mask = ~mask;
        return true;
    }
    if (_stricmp(nexttoken, "CPSR") == 0
        || _stricmp(nexttoken, "SPSR") == 0) {
        args = nextargs;
        mc->addr = buildArmMRSInsn(nexttoken[0] == 'S');
        mc->isInsn = 1;
        if (get_expression(&args, &mask))
            get_suppress(args, mc);
        mc->mask = ~mask;
        return true;
    }

    // Normal address watch
    if (!get_expression(&args, &mc->addr)) {
        ScriptError("Expected <addr>");
        return false;
    }

    uint32 size = 32;
    if (get_expression(&args, &mask) && get_expression(&args, &size))
        get_suppress(args, mc);
    mc->mask = ~mask;

    switch (size) {
    case 32: mc->readSize=MO_SIZE32; break;
    case 16: mc->readSize=MO_SIZE16; break;
    case 8: mc->readSize=MO_SIZE8; break;
    default:
        ScriptError("Expected <32|16|8>");
        return false;
    }

    if (((mc->addr >> mc->readSize) << mc->readSize) != mc->addr) {
        ScriptError("Address %08x is not aligned for %d-bit accesses"
                    , mc->addr, size);
        return false;
    }

    return true;
}

void
watchListVar::showVar(const char *args)
{
    memcheck *mc = watchlist;
    for (uint i=0; i<watchcount; i++, mc++) {
        char cmpBuf[32];
        if (mc->isInsn)
            Output("%2d: insn %08x %08x %s"
                   , i, mc->insn, ~mc->mask, disp_suppress(mc, cmpBuf));
        else
            Output("%2d: 0x%08x %08x %2d %s"
                   , i, mc->addr, ~mc->mask, 8<<mc->readSize
                   , disp_suppress(mc, cmpBuf));
    }
}

// Output the addresses to be watched.
void
watchListVar::beginWatch(int isStart)
{
    if (isStart)
        Output("Beginning memory tracing.");
    memcheck *mc = watchlist;
    for (uint i=0; i<watchcount; i++, mc++) {
        mc->trySuppress = 0;
        if (mc->isInsn) {
            Output("Watching %s(%02d): Insn %08x"
                   , name, i, mc->insn);
        } else {
            uint32 paddr = memVirtToPhys(mc->addr);
            Output("Watching %s(%02d): Addr %08x(@%08x)"
                   , name, i, mc->addr, paddr);
        }
    }
}

void
watchListVar::reportWatch(const char *header, uint32 pos
                          , uint32 newval, uint32 changed, uint32 pc)
{
    memcheck *mc = &watchlist[pos];
    const char *atype = "mem";
    if (mc->isInsn)
        atype = "insn";

    char pcbuf[32];
    const char *pcstr = "";
    if (pc) {
        _snprintf(pcbuf, sizeof(pcbuf), " @~%08x", pc);
        pcstr = pcbuf;
    }

    Output("%s %s %s(%d) %08x=%08x (%08x)%s"
           , header, atype, name, pos
           , mc->addr, newval, changed, pcstr);
}

static watchListVar *
FindWatchVar(const char **args)
{
    char varname[32];
    if (get_token(args, varname, sizeof(varname))) {
        ScriptError("Expected <varname>");
        return NULL;
    }
    watchListVar *wl = watchListVar::cast(FindVar(varname));
    if (!wl) {
        ScriptError("Expected <watch list var>");
        return NULL;
    }
    return wl;
}

// Helper for handling memory poll haret commands.
static void
ignoreBit(const char *cmd, const char *args)
{
    watchListVar *wl = FindWatchVar(&args);
    if (!wl)
        return;

    // Add bits to the mask
    uint32 start, end;
    while (get_range(&args, &start, &end)) {
        uint32 max = wl->watchcount * 32 - 1;
        if (start > max) {
            Output(C_ERROR "Bit %d is past max of %d", end, max);
            return;
        }
        if (end > max)
            end = max;
        for (uint i=start; i<=end; i++) {
            uint pos = i / 32;
            uint mask = 1<<(i % 32);
            if (toupper(cmd[0]) == 'I')
                wl->watchlist[pos].mask &= ~mask;
            else
                wl->watchlist[pos].mask |= mask;
        }
    }
}
REG_CMD(0, "IBIT", ignoreBit,
        "IBIT <watch list> <bit range> [<bit range>...]\n"
        "  Ignore bits in a watch list\n"
        "  A bit range can be a number (eg, 1) or a range (eg, 1..5)")
REG_CMD_ALT(0, "WBIT", ignoreBit, watch,
            "WBIT <watch list> <bit range> [<bit range>...]\n"
            "  Opposite of IBIT - remove bits from mask")


/****************************************************************
 * Basic memory polling.
 ****************************************************************/

REG_VAR_WATCHLIST(
    0, "GPIOS", GPIOS,
    "List of GPIOs to watch\n"
    "  List of <addr> [<mask> [<32|16|8> [<ignVal>]]] 4-tuples.\n"
    "  OR      CP <cp#> <op1> <CRn> <CRm> <op2> [<mask> [<ignVal>]] 7-tuples\n"
    "  OR      [C|S]PSR [<mask> [<ignVal>]] 2-tuples\n"
    "    <addr>    is a virtual address to watch (can use P2V(physaddr))\n"
    "    <mask>    is a bitmask to ignore when detecting a change (default 0)\n"
    "    <32|16|8> specifies the memory access type (default 32)\n"
    "    <ignVal>  report only when read value doesn't equal this value - one\n"
    "              may also specify 'last' or 'none' (default is 'last'\n"
    "              - report on change)\n"
    "  One may watch either a memory address or an internal register")

static void
cmd_watch(const char *cmd, const char *args)
{
    watchListVar *wl = FindWatchVar(&args);
    if (!wl)
        return;

    uint32 seconds;
    if (!get_expression(&args, &seconds))
        seconds = 0;

    wl->beginWatch();

    uint32 start_time = GetTickCount();
    uint32 cur_time = start_time;
    uint32 fin_time = cur_time + seconds * 1000;

    for (;;) {
        for (uint i=0; i<wl->watchcount; i++) {
            memcheck *mc = &wl->watchlist[i];
            uint32 val, changed;
            int ret = testMem(mc, &val, &changed);
            if (!ret)
                continue;
            char header[64];
            _snprintf(header, sizeof(header), "%06d:", cur_time - start_time);
            wl->reportWatch(header, i, val, changed);
        }

        cur_time = GetTickCount();
        if (cur_time >= fin_time)
            break;
        late_SleepTillTick();
    }
}
REG_CMD(0, "W|ATCH", cmd_watch,
        "WATCH <watch list> [<seconds>]\n"
        "  Poll areas of memory and report changes.  (See var GPIOS)")
