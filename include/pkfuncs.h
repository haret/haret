#ifndef __PKFUNCS_H
#define __PKFUNCS_H

// The following definitions are from:
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcemain4/html/cmconKernelFunctions.asp
//

#ifdef __cplusplus
extern "C" {
#endif

VOID ForcePageout(void);
LPVOID AllocPhysMem(DWORD cbSize, DWORD fdwProtect, DWORD dwAlignmentMask,
                    DWORD dwFlags, PULONG pPhysicalAddress);
BOOL FreePhysMem(LPVOID lpvAddress);
DWORD SetProcPermissions(DWORD newperms );
DWORD GetCurrentPermissions(void);
BOOL SetKMode(BOOL fMode);
BOOL VirtualCopy(LPVOID lpvDest, LPVOID lpvSrc, DWORD cbSize, DWORD fdwProtect);
BOOL LockPages(LPVOID lpvAddress, DWORD cbSize, PDWORD pPFNs, int fOptions);
BOOL UnlockPages(LPVOID lpvAddress, DWORD cbSize);
#define LOCKFLAG_READ       0x004

#ifdef __cplusplus
} // extern "C"
#endif

#endif // pkfuncs.h
