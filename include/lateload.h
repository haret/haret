#ifndef __LATELOAD_H
#define __LATELOAD_H

// Late loading of library calls.

// Create a pointer called "late_XXX" that points to a function XXX if
// it can be succesfully pulled in from the given DLL at runtime.
// Otherwise it points to NULL.
#define LATE_LOAD(Func, DLL) \
    __LATE_LOAD(Func, L ## #Func, L ##DLL, 0)

// As above, but have the pointer point to the function "alt_XXX" if
// the real function can't be pulled in from the dll.
#define LATE_LOAD_ALT(Func, DLL) \
    __LATE_LOAD(Func, L ## #Func, L ##DLL, & alt_ ##Func )

// Use this to refer to lateload defined in another module.
#define EXTERN_LATE_LOAD(Func) \
    extern typeof(&Func) late_ ##Func;

//
// Internal definitions.
//

#define __LATE_LOAD(Func, Name, DLL, Alt)                       \
    typeof(&Func) late_ ##Func;                                 \
    struct late_load_s LateLoad ##Func                          \
        __attribute__((__section__ (".rdata.late"))) = {        \
        DLL, Name, (void **) & late_ ##Func , (void*) Alt };

struct late_load_s {
    const wchar_t *dll, *funcname;
    void **funcptr;
    void *alt;
};

void setup_LateLoading();

#endif // lateload.h
