#ifndef __PKFUNCS_H
#define __PKFUNCS_H

// The following definitions are from:
//
// http://msdn.microsoft.com/library/default.asp?url=/library/en-us/wcemain4/html/cmconKernelFunctions.asp
//

extern "C" {

VOID ForcePageout(void);
LPVOID AllocPhysMem(DWORD cbSize, DWORD fdwProtect, DWORD dwAlignmentMask,
                    DWORD dwFlags, PULONG pPhysicalAddress);
BOOL FreePhysMem(LPVOID lpvAddress);
DWORD SetProcPermissions(DWORD newperms );
DWORD GetCurrentPermissions(void);
BOOL SetKMode(BOOL fMode);
BOOL VirtualCopy(LPVOID lpvDest, LPVOID lpvSrc, DWORD cbSize, DWORD fdwProtect);

} // extern "C"

#endif // pkfuncs.h
