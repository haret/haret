/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "xtypes.h"
#include "output.h" // Output
#include "script.h" // REG_DUMP

extern "C" uint32 _cpu_get_cp (uint32 cp, uint32 regno);
extern "C" void _cpu_set_cp (uint32 cp, uint32 regno, uint32 val);

// Read one register of coprocessor
uint32 cpuGetCP (uint cp, uint regno)
{
  if (cp > 15)
    return 0xffffffff;

  uint32 value;
  try
  {
    value = _cpu_get_cp (cp, regno);
  }
  catch (...)
  {
    Output(C_ERROR "EXCEPTION reading coprocessor %d register %d", cp, regno);
    value = 0xffffffff;
  }

  return value;
}

// Set a coprocessor register
bool cpuSetCP (uint cp, uint regno, uint32 val)
{
  if (cp > 15)
    return false;

  bool rc = true;
  try
  {
    _cpu_set_cp (cp, regno, val);
  }
  catch (...)
  {
    Output(C_ERROR "EXCEPTION writing to coprocessor %d register %d", cp, regno);
    rc = false;
  }

  return rc;
}

static bool
cpuDumpCP(uint32 *args)
{
  uint cp = args [0];

  if (cp > 15)
  {
    Output(C_ERROR "Coprocessor number is a number in range 0..15");
    return false;
  }

  for (int i = 0; i < 8; i++)
    Output("c%02d: %08x | c%02d: %08x",
         i, cpuGetCP (cp, i), i + 8, cpuGetCP (cp, i + 8));
  return true;
}
REG_DUMP(0, "CP", cpuDumpCP, 1,
         "Value of 16 coprocessor registers (arg = coproc number)")

static uint32 cpuScrCP (bool setval, uint32 *args, uint32 val)
{
  if (setval)
    return cpuSetCP (args [0], args [1], val) ? 0 : -1;
  return cpuGetCP (args [0], args [1]);
}
REG_VAR_RWFUNC(0, "CP", cpuScrCP, 2, "Coprocessor Registers access")
