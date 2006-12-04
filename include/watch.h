#ifndef __WATCH_H
#define __WATCH_H

// Mark a function to be available during interrupt handling.  (See
// irq.cpp for more details.)
#define __irq __attribute__ ((__section__ (".text.irq")))

// Function callback definition to report the results of a memory
// check.
typedef void (*reporter_t)(uint32 clock, struct memcheck *, uint32 newval);

// Main definition of a memory polling request.
struct memcheck {
    reporter_t reporter;
    void *data;
    uint32 readSize : 3;
    uint32 trySuppress : 1, setCmp : 1, trySuppressNext : 1;
    char *addr;
    uint32 cmpVal;
    uint32 mask;
};

// Read a memory area and check for a change.
int __irq testMem(struct memcheck *mc, uint32 *pnewval);

// Helper for handling memory poll haret commands.
void watchCmdHelper(memcheck *list, uint32 max, uint32 *total
                    , const char *cmd, const char *args);

// Give up rest of time slice.  This calls SleepTillTick() if it is
// available - otherwise Sleep(1).
void yieldCPU();

#endif // watch.h
