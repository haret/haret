/*
    Utility functions
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>
#include <time.h>

// Replacement for POSIX time() which is missing in PPC 2002
extern "C" time_t time (time_t *)
{
  SYSTEMTIME st;
  FILETIME ft;
  GetSystemTime (&st);
  SystemTimeToFileTime (&st, &ft);
  // Approximate division by 10000000 by right-shifting the result 23 times.
  time_t res = (ft.dwLowDateTime >> 23) | (ft.dwHighDateTime << 9);
  return res;
}
