#ifndef __PKFUNCS_H
#define __PKFUNCS_H

// The following definitions are from:
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcemain4/html/cmconKernelFunctions.asp
//

#include <winioctl.h> // CTL_CODE
#ifndef FILE_DEVICE_HAL
# define FILE_DEVICE_HAL 0x00000101
#endif

#ifdef __cplusplus
extern "C" {
#endif

LPVOID AllocPhysMem(DWORD,DWORD,DWORD,DWORD,PULONG);
VOID ForcePageout(void);
BOOL FreePhysMem(LPVOID);
DWORD GetCurrentPermissions(void);
BOOL KernelIoControl(DWORD,LPVOID,DWORD,LPVOID,DWORD,LPDWORD);
BOOL LockPages(LPVOID,DWORD,PDWORD,int);
BOOL SetKMode(BOOL);
DWORD SetProcPermissions(DWORD);
void SleepTillTick();
BOOL UnlockPages(LPVOID,DWORD);
BOOL VirtualCopy(LPVOID,LPVOID,DWORD,DWORD);
#define LOCKFLAG_WRITE 0x001
#define LOCKFLAG_READ  0x004

#define IOCTL_PROCESSOR_INFORMATION CTL_CODE(FILE_DEVICE_HAL, 25, METHOD_BUFFERED, FILE_ANY_ACCESS)

typedef struct __PROCESSOR_INFO {
    WORD wVersion;
    WCHAR szProcessCore[40];
    WORD wCoreRevision;
    WCHAR szProcessorName[40];
    WORD wProcessorRevision;
    WCHAR szCatalogNumber[100];
    WCHAR szVendor[100];
    DWORD dwInstructionSet;
    DWORD dwClockSpeed;
} PROCESSOR_INFO, *PPROCESSOR_INFO;


#ifdef __cplusplus
} // extern "C"
#endif

#endif // pkfuncs.h
