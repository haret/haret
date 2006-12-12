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
#include "watch.h"

LATE_LOAD(SleepTillTick, "coredll")

void
yieldCPU()
{
    if (late_SleepTillTick)
        late_SleepTillTick();
    else
        Sleep(1);
}

// Types of memory accesses.
enum MemOps {
    MO_READ8 = 1,
    MO_READ16 = 2,
    MO_READ32 = 3,
};

int __irq
testMem(struct memcheck *mc, uint32 *pnewval)
{
    uint32 curval;
    switch (mc->readSize) {
    default:
    case MO_READ32:
        curval = *(uint32*)mc->addr;
        break;
    case MO_READ16:
        curval = *(uint16*)mc->addr;
        break;
    case MO_READ8:
        curval = *(uint8*)mc->addr;
        break;
    }

    if (mc->trySuppress && (curval & mc->mask) == (mc->cmpVal & mc->mask))
        // No change in value.
        return 0;
    if (mc->setCmp)
        mc->cmpVal = curval;
    mc->trySuppress = mc->trySuppressNext;
    *pnewval = curval;
    return 1;
}


/****************************************************************
 * Helper for registering watched memory
 ****************************************************************/

void
r_basic(uint32 clock, struct memcheck *mc, uint32 newval)
{
    Output("%10d: %p=%08x", clock, mc->addr, newval);
}

void
watchCmdHelper(memcheck *list, uint32 max, uint32 *ptotal
               , const char *cmd, const char *args)
{
    if (toupper(cmd[0]) == 'C') {
        // Clear list.
        *ptotal = 0;
        return;
    }
    uint32 total = *ptotal;
    if (toupper(cmd[0]) == 'L') {
        // List what is currently registered
        for (uint i=0; i<total; i++) {
            memcheck *mc = &list[i];
            Output("%2d: 0x%p %08x %2d"
                   , i, mc->addr, ~mc->mask, 4<<mc->readSize);
        }
        return;
    }

    // Add a new item to the list.

    if (total >= max) {
        Output("Already at max (%d)", max);
        return;
    }

    // Parse args
    uint32 addr;
    if (!get_expression(&args, &addr)) {
        Output(C_ERROR "line %d: Expected <addr>", ScriptLine);
        return;
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
        Output(C_ERROR "line %d: Expected <32|16|8>", ScriptLine);
        return;
    }

    // Update structure.
    struct memcheck *mc = &list[total];
    *ptotal = total + 1;
    memset(mc, 0, sizeof(*mc));
    mc->reporter = r_basic;
    mc->addr = (char *)addr;
    mc->mask = ~mask;
    mc->readSize = size;
    mc->trySuppressNext = 1;
    mc->setCmp = !hasComp;
    mc->cmpVal = cmpVal;
}


/****************************************************************
 * Basic memory polling.
 ****************************************************************/

static uint32 watchcount;
static memcheck watchlist[16];

static void
cmd_addwatch(const char *cmd, const char *args)
{
    watchCmdHelper(watchlist, ARRAY_SIZE(watchlist), &watchcount, cmd, args);
}
REG_CMD(0, "ADDWATCH", cmd_addwatch,
        "ADDWATCH <addr> [<mask> <32|16|8> <cmpValue>]\n"
        "  Setup an address to be watched (via WATCH)\n"
        "  <addr>     is a virtual address to watch (can use P2V(physaddr))\n"
        "  <mask>     is a bitmask to ignore when detecting a change\n"
        "  <32|16|8>  specifies the memory access type\n"
        "  <cmpValue> when specified forces a report if read value doesn't\n"
        "             equal that value")
REG_CMD_ALT(0, "CLEARWATCH", cmd_addwatch, clear,
            "CLEARWATCH\n"
            "  Remove all items from the list of polled memory");
REG_CMD_ALT(0, "LSWATCH", cmd_addwatch, list,
            "LSWATCH\n"
            "  Display the current list of polled memory");

static void
cmd_watch(const char *cmd, const char *args)
{
    uint32 seconds;
    if (!get_expression(&args, &seconds))
        seconds = 0;

    int cur_time = time(NULL);
    int fin_time = cur_time + seconds;

    for (;;) {
        uint32 clock = GetTickCount();
        for (uint i=0; i<watchcount; i++) {
            memcheck *mc = &watchlist[i];
            uint32 val;
            int ret = testMem(mc, &val);
            if (!ret)
                continue;
            mc->reporter(clock, mc, val);
        }

        cur_time = time(NULL);
        if (cur_time >= fin_time)
            break;
        yieldCPU();
    }
}
REG_CMD(0, "WATCH", cmd_watch,
        "WATCH <seconds>\n"
        "  Poll pre-registered areas of memory (see ADDWATCH)\n"
        "  and report changes.")
