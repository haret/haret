/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>

#include "xtypes.h"
#include "cpu.h"
#include "output.h"
#include "haret.h"

extern "C" uint32 _cpu_get_cp (uint32 cp, uint32 regno);
extern "C" void _cpu_set_cp (uint32 cp, uint32 regno, uint32 val);

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
    Complain (C_ERROR ("EXCEPTION reading coprocessor %d register %d"), cp, regno);
    value = 0xffffffff;
  }

  return value;
}

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
    Complain (C_ERROR ("EXCEPTION writing to coprocessor %d register %d"), cp, regno);
    rc = false;
  }

  return rc;
}
