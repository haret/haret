#ifndef __WATCH_H
#define __WATCH_H

#include "script.h" // listVarBase

// Main definition of a memory polling request.
struct memcheck {
    uint32 isCP : 1;
    uint32 readSize : 2;
    uint32 trySuppress : 1, setCmp : 1, trySuppressNext : 1;
    uint32 cmpVal;
    uint32 mask;
    union {
        char *addr;
        uint32 insn;
    };
};

int testMem(struct memcheck *mc, uint32 *pnewval, uint32 *pmaskval);
void reportWatch(uint32 msecs, uint32 clock, struct memcheck *mc
                 , uint32 newval, uint32 maskval);

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
