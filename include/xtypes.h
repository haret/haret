/*
    eXtended types and definitions
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _XTYPES_H
#define _XTYPES_H

#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))

typedef signed char int8;
typedef unsigned char uint8;
typedef signed short int16;
typedef unsigned short uint16;
typedef signed int int32;
typedef unsigned int uint32;

#ifdef __GNUC__
typedef signed long long int64;
typedef unsigned long long uint64;
#else
typedef __int64 int64;
typedef unsigned __int64 uint64;
#endif

/* Shorter version of "unsigned xxx" types */
typedef unsigned char uchar;
typedef unsigned short ushort;
typedef unsigned int uint;
typedef unsigned long ulong;

#endif /* _XTYPES_H */
