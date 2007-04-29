#ifndef __WATCH_H
#define __WATCH_H

#include "script.h" // listVarBase

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

int testMem(struct memcheck *mc, uint32 *pnewval, uint32 *pmaskval);

// Give up rest of time slice.
extern void (*late_SleepTillTick)();

// Trace listing class
class watchListVar : public listVarBase {
public:
    uint32 watchcount;
    memcheck watchlist[64];
    watchListVar(predFunc ta, const char *n, const char *d)
        : listVarBase(ta, n, d, &watchcount, (void*)watchlist
                      , sizeof(watchlist[0]), ARRAY_SIZE(watchlist)) { }
    bool setVarItem(void *p, const char *args);
    void showVar(const char *args);
    void fillVarType(char *buf);
    void beginWatch(int isStart=1);
};
#define REG_VAR_WATCHLIST(Pred, Name, Var, Desc)       \
    __REG_VAR(watchListVar, Var, Pred, Name, Desc)

#endif // watch.h
