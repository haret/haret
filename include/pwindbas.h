#ifndef __PWINDBAS_H
#define __PWINDBAS_H

#ifdef __cplusplus
extern "C" {
#endif

WINBASEAPI DWORD SetSystemMemoryDivision(DWORD);
#define SYSMEM_CHANGED 0
#define SYSMEM_MUSTREBOOT 1
#define SYSMEM_REBOOTPENDING 2
#define SYSMEM_FAILED 3

#ifdef __cplusplus
} // extern "C"
#endif

#endif // pwindbas.h
