/*
    Handheld Reverse Engineering Tool
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _HARET_H
#define _HARET_H

#include <windows.h>

extern HINSTANCE hInst;

// Some half-documented API functions (absent in SDK headers)
extern "C" DWORD SetProcPermissions (DWORD newperms);
extern "C" DWORD GetCurrentPermissions ();
extern "C" BOOL SetKMode (BOOL fMode);
extern "C" BOOL VirtualCopy (LPVOID lpvDestMem, LPVOID lpvSrcMem,
  DWORD dwSizeInBytes, DWORD dwProtectFlag);

#endif /* _HARET_H */
