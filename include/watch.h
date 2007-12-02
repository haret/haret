#ifndef __WATCH_H
#define __WATCH_H

#include "script.h" // listVarBase

// Main definition of a memory polling request.
struct memcheck {
    uint32 isInsn : 1;
    uint32 readSize : 2;
    uint32 trySuppress : 1, setCmp : 1, trySuppressNext : 1;
    uint32 rw : 2;
    uint32 cmpVal;
    uint32 mask;
    union {
        uint32 addr;
        uint32 insn;
    };
    uint32 endaddr;
};

int testChanged(struct memcheck *mc, uint32 curval, uint32 *pchanged);
int testMem(struct memcheck *mc, uint32 *pnewval, uint32 *pchanged);
void reportWatch(uint32 msecs, uint32 clock, struct memcheck *mc
                 , uint32 newval, uint32 changed);
void get_suppress(const char *args, memcheck *mc);
const char *disp_suppress(memcheck *mc, char *buf);

// Give up rest of time slice.
extern void (*late_SleepTillTick)();

// Trace listing class
class watchListVar : public listVarBase {
public:
    uint32 watchcount;
    memcheck watchlist[64];
    watchListVar(predFunc ta, const char *n, const char *d)
        : listVarBase(ta, n, d, &watchcount, (void*)watchlist
                      , sizeof(watchlist[0]), ARRAY_SIZE(watchlist))
        , watchcount(0) { }
    variableBase *newVar() { return new watchListVar(0, "", ""); }
    bool setVarItem(void *p, const char *args);
    void showVar(const char *args);
    void fillVarType(char *buf);
    void beginWatch(int isStart=1);
};
#define REG_VAR_WATCHLIST(Pred, Name, Var, Desc)       \
    __REG_VAR(watchListVar, Var, Pred, Name, Desc)

#endif // watch.h
