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

// Self-modified code
static uint32 selfmod [2] =
{
  0xee100010,	// mrc pX,0,r0,crX,cr0,0
  0xe1a0f00e    // mov pc,lr
};

static bool FlushSelfMod (const char *op)
{
  bool rc = true;
  __try
  {
    SetKMode (TRUE);
    cli ();
    cpuFlushCache ();
    sti ();
    SetKMode (FALSE);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Complain (C_ERROR ("EXCEPTION while preparing to %hs coprocessor"), op);
    rc = false;
  }
  return rc;
}

uint32 cpuGetCP (uint cp, uint regno)
{
  if (cp > 15)
    return 0xffffffff;

  uint32 value;
  selfmod [0] = 0xee100010 | (cp << 8) | (regno << 16);

  if (!FlushSelfMod ("read"))
    return 0xffffffff;

  __try
  {
    value = ((uint32 (*) ())&selfmod) ();
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
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

  selfmod [0] = 0xee000f10 | (cp << 8) | (regno << 16);
  if (!FlushSelfMod ("write"))
    return false;

  bool rc = true;
  __try
  {
    ((void (*) (uint32))&selfmod) (val);
  }
  __except (EXCEPTION_EXECUTE_HANDLER)
  {
    Complain (C_ERROR ("EXCEPTION writing to coprocessor %d register %d"), cp, regno);
    rc = false;
  }

  return rc;
}
