/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <string.h>
#include <stdlib.h>

#include "xtypes.h"
#include "cpu.h"
#include "output.h"
#include "haret.h"
#include "memory.h"
#include "util.h"

static bool unknownDetect ()
{ return true; }

static int unknownSetupLoad ()
{ return 0; }

static int unknownShutdownPeripherals ()
{ return 0; }

static int unknownAttemptRecovery ()
{ return 0; }

struct cpu_fns *cpu = NULL;
static cpu_fns cpu_unknown =
{
  L"Unknown",
  unknownDetect,
  unknownSetupLoad,
  unknownShutdownPeripherals,
  unknownAttemptRecovery
};

extern struct cpu_fns cpu_pxa;
extern struct cpu_fns cpu_s3c24xx;

static struct cpu_fns *cpus [] =
{
  &cpu_pxa,
  &cpu_s3c24xx,
  &cpu_unknown
};

void cpuDetect ()
{
  for (int i = 0; i < sizeof (cpus) / sizeof (cpus [0]); i++)
    if (cpus [i]->detect ())
    {
      cpu = cpus [i];
      Log (L"Detected CPU family: %s", cpu->name);
      return;
    }
  Complain (C_ERROR ("Cannot detect the family of your CPU:\n"
                    L"some functions will not be available"));
}

uint32 cpuGetFamily (bool setval, uint32 *args, uint32 val)
{
  return (uint32)cpu->name;
}

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
    out (data, "c%02d: %08x | c%02d: %08x\n",
         i, cpuGetCP (cp, i), i + 8, cpuGetCP (cp, i + 8));
  return true;
}

bool cpuDumpAC97 (void (*out) (void *data, const char *, ...),
                  void *data, uint32 *args)
{
  uint unit = args [0];

  if (unit > 3)
  {
    Complain (C_ERROR ("AC97 unit number must be between 0 or 3"));
    return false;
  }

  pxaAC97 volatile *ac97 = (pxaAC97 *)memPhysMap (AC97_BASE);
  if (!ac97)
  {
    Complain (C_ERROR ("Cannot map AC97 controller's physical memory"));
    return false;
  }

  uint16 regs [64];

  // Disable AC97 interrupt generation
  uint32 old_gcr = ac97->GCR;
  ac97->GCR = old_gcr & ~(GCR_PRIRDY_IEN | GCR_SECRDY_IEN | GCR_SDONE_IE |
                          GCR_CDONE_IE | GCR_GIE);
  uint32 old_pocr = ac97->POCR;
  ac97->POCR = old_pocr & ~POCR_FEIE;
  uint32 old_picr = ac97->PICR;
  ac97->PICR = old_picr & ~PICR_FEIE;
  uint32 old_mccr = ac97->MCCR;
  ac97->MCCR = old_mccr & ~MCCR_FEIE;
  uint32 old_mocr = ac97->MOCR;
  ac97->MOCR = old_mocr & ~MOCR_FEIE;
  uint32 old_micr = ac97->MICR;
  ac97->MICR = old_micr & ~MICR_FEIE;

  int i;
  for (i = 0; i < 64; i++)
  {
    uint32 volatile *reg = &ac97->codec [unit][i];

    // In the case of error ...
    regs [i] = 0xffff;

    cli ();

    ac97->CAR &= ~CAR_CAIP;
    int to = 10000;

    // First read the Codec Access Register
    while ((ac97->CAR & CAR_CAIP) && --to)
      ;
    if (!to)
    {
      sti ();
      out (data, "Register %x: codec is busy\n", i * 2);
      continue;
    }

    // A dummy read from register (results in invalid data)
    (void)*reg;

    // Drop the SDONE and RDCS bits
    ac97->GSR |= GSR_SDONE | GSR_RDCS;

    // Wait for the SDONE bit
    while (!(ac97->GSR & (GSR_SDONE | GSR_RDCS)) && --to)
      ;
    if (!to || (ac97->GSR & GSR_RDCS))
    {
      sti ();
      out (data, "Register %x: access timed out\n", i * 2);
      continue;
    }

    regs [i] = *reg;

    sti ();

    // Shit, if we remove this it won't work correctly :-(
    out (data, ".\b");
  }
  out (data, "\n");

  ac97->POCR = old_pocr;
  ac97->PICR = old_picr;
  ac97->MCCR = old_mccr;
  ac97->MOCR = old_mocr;
  ac97->MICR = old_micr;
  ac97->GCR = old_gcr;

  out (data, "GCR:  %08x  MCCR: %08x\n", old_gcr,  old_mccr);
  out (data, "POCR: %08x  PICR: %08x\n", old_pocr, old_picr);
  out (data, "MOCR: %08x  MICR: %08x\n", old_mocr, old_micr);

  for (i = 0; i < 16; i++)
    out (data, "r%02x: %04x | r%02x: %04x | r%02x: %04x | r%02x: %04x\n",
          i       * 2, regs [i     ], (i + 16) * 2, regs [i + 16],
         (i + 32) * 2, regs [i + 32], (i + 48) * 2, regs [i + 48]);
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
