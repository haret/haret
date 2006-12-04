#ifndef __WATCH_H
#define __WATCH_H

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

void watchCmdHelper(memcheck *list, uint32 max, uint32 *total
                    , const char *cmd, const char *args);

#endif // watch.h
