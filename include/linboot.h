#include <stdio.h> // FILE
#include "xtypes.h" // uint32

// Mark a function that is used in the C preloader.  Note all
// functions marked this way will be copied to physical ram for the
// preloading and are run with the MMU disabled.  These functions must
// be careful to not call functions that aren't also marked this way.
// They must also not use any global variables.
#define __preload __attribute__ ((__section__ (".text.preload")))

void __preload do_copy(char *dest, const char *src, int count);

void bootRamLinux(const char *kernel, uint32 kernelSize
                  , const char *initrd, uint32 initrdSize
                  , int bootViaResume=0);
void bootHandleLinux(FILE *f, int kernelSize, int initrdSize, 
		     int bootViaResume=0);
