#ifndef __LATELOAD_H
#define __LATELOAD_H

// Late loading of library calls.

#define LATE_LOAD(Func, DLL) __LATE_LOAD(Func, L ## #Func, L ##DLL)

#define __LATE_LOAD(Func, Name, DLL)                            \
    typeof(&Func) late_ ##Func;                                 \
    struct late_load_s LateLoad ##Func                          \
        __attribute__((__section__ (".rdata.late"))) = {        \
            DLL, Name, (void **) & late_ ##Func };

struct late_load_s {
    const wchar_t *dll, *funcname;
    void **funcptr;
};

void setup_LateLoading();

#endif // lateload.h
