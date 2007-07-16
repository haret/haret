#ifndef __PKFUNCS_H
#define __PKFUNCS_H

// The following definitions are from:
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcemain4/html/cmconKernelFunctions.asp
//

#ifdef __cplusplus
extern "C" {
#endif

LPVOID AllocPhysMem(DWORD,DWORD,DWORD,DWORD,PULONG);
VOID ForcePageout(void);
BOOL FreePhysMem(LPVOID);
DWORD GetCurrentPermissions(void);
BOOL LockPages(LPVOID,DWORD,PDWORD,int);
BOOL SetKMode(BOOL);
DWORD SetProcPermissions(DWORD);
void SleepTillTick();
BOOL UnlockPages(LPVOID,DWORD);
BOOL VirtualCopy(LPVOID,LPVOID,DWORD,DWORD);
#define LOCKFLAG_WRITE 0x001
#define LOCKFLAG_READ  0x004

WINBASEAPI DWORD SetSystemMemoryDivision(DWORD);
#define SYSMEM_CHANGED 0
#define SYSMEM_MUSTREBOOT 1
#define SYSMEM_REBOOTPENDING 2
#define SYSMEM_FAILED 3

#ifdef __cplusplus
} // extern "C"
#endif

#endif // pkfuncs.h
