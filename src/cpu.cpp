/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "xtypes.h"
#include "cpu.h"
#include "output.h"
#include "haret.h"

bool cpuDumpCP (void (*out) (void *data, const char *, ...),
                void *data, uint32 *args)
{
  uint cp = args [0];

  if (cp > 15)
  {
    Complain (C_ERROR ("Coprocessor number is a number in range 0..15"));
    return false;
  }

  for (int i = 0; i < 8; i++)
    out (data, "c%02d: %08x  c%02d: %08x\n",
         i, cpuGetCP (cp, i), i + 8, cpuGetCP (cp, i + 8));
  return true;
}

// Returns the address of 1st level descriptor table
uint32 cpuGetMMU ()
{
  return cpuGetCP (15, 2) & 0xffffc000;
}

// Returns the PID register contents
uint32 cpuGetPID ()
{
  return cpuGetCP (15, 13) >> 25;
}

uint32 cpuScrCP (bool setval, uint32 *args, uint32 val)
{
  if (setval)
    return cpuSetCP (args [0], args [1], val) ? 0 : -1;
  return cpuGetCP (args [0], args [1]);
}
