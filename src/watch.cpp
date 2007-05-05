/* Poll areas of memory for changes.
 *
 * (C) Copyright 2006 Kevin O'Connor <kevin@koconnor.net>
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include "windows.h" // Sleep
#include "pkfuncs.h" // SleepTillTick
#include <ctype.h> // toupper
#include <time.h> // time

#include "output.h" // Output
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

// Types of memory accesses.
enum MemOps {
    MO_READ8 = 1,
    MO_READ16 = 2,
    MO_READ32 = 3,
};

static inline uint32 __irq
doRead(struct memcheck *mc)
{
    switch (mc->readSize) {
    default:
    case MO_READ32:
        return *(uint32*)mc->addr;
    case MO_READ16:
        return *(uint16*)mc->addr;
    case MO_READ8:
        return *(uint8*)mc->addr;
    }
}

// Read a memory area and check for a change.
int __irq
testMem(struct memcheck *mc, uint32 *pnewval, uint32 *pmaskval)
{
    uint32 curval = doRead(mc);
    uint32 maskedval = 0;
    if (mc->trySuppress) {
        maskedval = (curval ^ mc->cmpVal) & mc->mask;
        if (maskedval == 0)
            // No change in value.
            return 0;
    }
    if (mc->setCmp)
        mc->cmpVal = curval;
    mc->trySuppress = mc->trySuppressNext;
    *pnewval = curval;
    *pmaskval = maskedval;
    return 1;
}


/****************************************************************
 * Helpers for registering watched memory
 ****************************************************************/

static void
r_basic(uint32 msecs, uint32 clock, struct memcheck *mc
        , uint32 newval, uint32 maskval)
{
    if (clock != (uint32)-1)
        Output("%06d: %08x: mem %p=%08x (%08x)"
               , msecs, clock, mc->addr, newval, maskval);
    else
        Output("%06d: mem %p=%08x (%08x)", msecs, mc->addr, newval, maskval);
}

bool
watchListVar::setVarItem(void *p, const char *args)
{
    // Parse args
    uint32 addr;
    if (!get_expression(&args, &addr)) {
        ScriptError("Expected <addr>");
        return false;
    }
    uint32 mask = 0, size = 32, hasComp=0, cmpVal=0;
    if (get_expression(&args, &mask) && get_expression(&args, &size)
        && get_expression(&args, &cmpVal))
        hasComp = 1;
    switch (size) {
    case 32: size=MO_READ32; break;
    case 16: size=MO_READ16; break;
    case 8: size=MO_READ8; break;
    default:
        ScriptError("Expected <32|16|8>");
        return false;
    }

    // Update structure.
    memcheck *mc = (memcheck*)p;
    memset(mc, 0, sizeof(*mc));
    mc->reporter = r_basic;
    mc->addr = (char *)addr;
    mc->mask = ~mask;
    mc->readSize = size;
    mc->trySuppressNext = 1;
    mc->setCmp = !hasComp;
    mc->cmpVal = cmpVal;
    return true;
}

void
watchListVar::showVar(const char *args)
{
    memcheck *mc = watchlist;
    for (uint i=0; i<watchcount; i++, mc++)
        Output("%2d: 0x%p %08x %2d"
               , i, mc->addr, ~mc->mask, 4<<mc->readSize);
}

void watchListVar::fillVarType(char *buf) {
    strcpy(buf, "watch list");
}

// Output the addresses to be watched.
void
watchListVar::beginWatch(int isStart)
{
    if (isStart)
        Output("Beginning memory tracing.");
    memcheck *mc = watchlist;
    for (uint i=0; i<watchcount; i++, mc++) {
        uint32 paddr = memVirtToPhys((uint32)mc->addr);
        mc->trySuppress = 0;
        uint32 val, tmp;
        testMem(mc, &val, &tmp);
        Output("Watching %s(%02d): Addr %p(@%08x) = %08x"
               , name, i, mc->addr, paddr, val);
    }
}

watchListVar *
FindWatchVar(const char **args)
{
    char varname[32];
    if (get_token(args, varname, sizeof(varname))) {
        ScriptError("Expected <varname>");
        return NULL;
    }
    watchListVar *wl = dynamic_cast<watchListVar*>(FindVar(varname));
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
    uint32 bit;
    while (get_expression(&args, &bit)) {
        uint pos = bit / 32;
        uint mask = 1<<(bit % 32);
        if (pos >= wl->watchcount) {
            Output(C_ERROR "Bit %d is past max found of %d"
                   , pos, wl->watchcount);
            return;
        }
        if (toupper(cmd[0]) == 'I')
            wl->watchlist[pos].mask &= ~mask;
        else
            wl->watchlist[pos].mask |= mask;
    }
}
REG_CMD(0, "IBIT", ignoreBit,
            "IBIT <watch list> <bit#> [<bit#>...]\n"
            "  Ignore the nth bit in the watch list");
REG_CMD_ALT(0, "WBIT", ignoreBit, watch,
            "WBIT <watch list> <bit#> [<bit#>...]\n"
            "  Opposite of IBIT - remove bit from mask");


/****************************************************************
 * Basic memory polling.
 ****************************************************************/

REG_VAR_WATCHLIST(
    0, "GPIOS", GPIOS,
    "List of GPIOs to watch\n"
    "  List of <addr> [<mask> [<32|16|8> [<cmpValue>]]] 4-tuples.\n"
    "    <addr>     is a virtual address to watch (can use P2V(physaddr))\n"
    "    <mask>     is a bitmask to ignore when detecting a change (default 0)\n"
    "    <32|16|8>  specifies the memory access type (default 32)\n"
    "    <cmpValue> when specified forces a report if read value doesn't\n"
    "               equal that value (default is to report on change)")

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
            uint32 val, maskval;
            int ret = testMem(mc, &val, &maskval);
            if (!ret)
                continue;
            mc->reporter(cur_time - start_time, -1, mc, val, maskval);
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
