#ifndef __WATCH_H
#define __WATCH_H

// Function callback definition to report the results of a memory
// check.
typedef void (*reporter_t)(uint32 msecs, uint32 clock, struct memcheck *
                           , uint32 newval, uint32 maskval);

// Main definition of a memory polling request.
struct memcheck {
    reporter_t reporter;
    void *data;
    uint32 readSize : 2;
    uint32 trySuppress : 1, setCmp : 1, trySuppressNext : 1;
    char *addr;
    uint32 cmpVal;
    uint32 mask;
};

// Read a memory area and check for a change.
int testMem(struct memcheck *mc, uint32 *pnewval, uint32 *pmaskval);

// Helper for handling memory poll haret commands.
void watchCmdHelper(memcheck *list, uint32 max, uint32 *total
                    , const char *cmd, const char *args);

// Give up rest of time slice.
extern void (*late_SleepTillTick)();

#endif // watch.h
