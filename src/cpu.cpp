/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "xtypes.h"
#include "output.h" // Complain
#include "memory.h" // memPhysMap
#include "pxa2xx.h" // pxaAC97
#include "machines.h" // Mach
#include "cpu.h"

uint32 cpuGetFamily (bool setval, uint32 *args, uint32 val)
{
  return (uint32)Mach->name;
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
  uint32 old_gcr = ac97->_GCR;
  ac97->_GCR = old_gcr & ~(GCR_PRIRDY_IEN | GCR_SECRDY_IEN | GCR_SDONE_IE |
                          GCR_CDONE_IE | GCR_GIE);
  uint32 old_pocr = ac97->_POCR;
  ac97->_POCR = old_pocr & ~POCR_FEIE;
  uint32 old_picr = ac97->_PICR;
  ac97->_PICR = old_picr & ~PICR_FEIE;
  uint32 old_mccr = ac97->_MCCR;
  ac97->_MCCR = old_mccr & ~MCCR_FEIE;
  uint32 old_mocr = ac97->_MOCR;
  ac97->_MOCR = old_mocr & ~MOCR_FEIE;
  uint32 old_micr = ac97->_MICR;
  ac97->_MICR = old_micr & ~MICR_FEIE;

  int i;
  for (i = 0; i < 64; i++)
  {
    uint32 volatile *reg = &ac97->codec [unit][i];

    // In the case of error ...
    regs [i] = 0xffff;

    cli ();

    ac97->_CAR &= ~CAR_CAIP;
    int to = 10000;

    // First read the Codec Access Register
    while ((ac97->_CAR & CAR_CAIP) && --to)
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
    ac97->_GSR |= GSR_SDONE | GSR_RDCS;

    // Wait for the SDONE bit
    while (!(ac97->_GSR & (GSR_SDONE | GSR_RDCS)) && --to)
      ;
    if (!to || (ac97->_GSR & GSR_RDCS))
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

  ac97->_POCR = old_pocr;
  ac97->_PICR = old_picr;
  ac97->_MCCR = old_mccr;
  ac97->_MOCR = old_mocr;
  ac97->_MICR = old_micr;
  ac97->_GCR = old_gcr;

  out (data, "GCR:  %08x  MCCR: %08x\n", old_gcr,  old_mccr);
  out (data, "POCR: %08x  PICR: %08x\n", old_pocr, old_picr);
  out (data, "MOCR: %08x  MICR: %08x\n", old_mocr, old_micr);

  for (i = 0; i < 16; i++)
    out (data, "r%02x: %04x | r%02x: %04x | r%02x: %04x | r%02x: %04x\n",
          i       * 2, regs [i     ], (i + 16) * 2, regs [i + 16],
         (i + 32) * 2, regs [i + 32], (i + 48) * 2, regs [i + 48]);
  return true;
}

DEF_GETCPR(get_p15r2, p15, 0, c2, c0, 0)

// Returns the address of 1st level descriptor table
uint32 cpuGetMMU ()
{
    return get_p15r2() & 0xffffc000;
}

DEF_GETCPR(get_p15r13, p15, 0, c13, c0, 0)

// Returns the PID register contents
uint32 cpuGetPID ()
{
    return get_p15r13() >> 25;
}

uint32 cpuScrCP (bool setval, uint32 *args, uint32 val)
{
  if (setval)
    return cpuSetCP (args [0], args [1], val) ? 0 : -1;
  return cpuGetCP (args [0], args [1]);
}
