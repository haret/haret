/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "windows.h"
#include "pkfuncs.h" // LockPages, SetKMode

#include "xtypes.h"
#include "output.h" // Complain
#include "memory.h" // memPhysMap
#include "pxa2xx.h" // pxaAC97
#include "machines.h" // Mach
#include "script.h" // REG_VAR_ROFUNC
#include "cpu.h"

REG_VAR_ROFUNC(0, "PSR", cpuGetPSR, 0, "Program Status Register")

static uint32
cpuGetFamily(bool setval, uint32 *args, uint32 val)
{
  return (uint32)Mach->name;
}
REG_VAR_ROFUNC(0, "CPU", cpuGetFamily, 0, "Autodetected CPU family")

static bool
cpuDumpCP(void (*out) (void *data, const char *, ...),
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
REG_DUMP(0, "CP", cpuDumpCP, 1,
         "Value of 16 coprocessor registers (arg = coproc number)")

static bool
cpuDumpAC97(void (*out) (void *data, const char *, ...),
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

    take_control();

    ac97->_CAR &= ~CAR_CAIP;
    int to = 10000;

    // First read the Codec Access Register
    while ((ac97->_CAR & CAR_CAIP) && --to)
      ;
    if (!to)
    {
      return_control();
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
      return_control();
      out (data, "Register %x: access timed out\n", i * 2);
      continue;
    }

    regs [i] = *reg;

    return_control();

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
REG_DUMP(0, "AC97", cpuDumpAC97, 1,
         "PXA AC97 ctrl (64x16-bit regs) (arg = ctrl number, 0..3).")

DEF_GETCPR(get_p15r2, p15, 0, c2, c0, 0)

// Returns the address of 1st level descriptor table
uint32 cpuGetMMU ()
{
    return get_p15r2() & 0xffffc000;
}
REG_VAR_ROFUNC(
    0, "MMU", cpuGetMMU, 0,
    "Memory Management Unit level 1 descriptor table physical addr")

DEF_GETCPR(get_p15r13, p15, 0, c13, c0, 0)

// Returns the PID register contents
uint32 cpuGetPID ()
{
    return get_p15r13() >> 25;
}

static uint32 cpuScrCP (bool setval, uint32 *args, uint32 val)
{
  if (setval)
    return cpuSetCP (args [0], args [1], val) ? 0 : -1;
  return cpuGetCP (args [0], args [1]);
}
REG_VAR_RWFUNC(0, "CP", cpuScrCP, 2, "Coprocessor Registers access")

// Symbols added by linker.
extern "C" {
    extern uint32 cpuFlushCache_data;
    extern uint32 cpuFlushCache_dataend;
}

static int controlCount;

// Take over CPU control from wince.  After calling this, the
// application should not be interrupted by any interrupts or faults.
// In general, the code should not make any OS calls until after
// return_control is invoked.
void
take_control()
{
    if (controlCount++)
        // Already in a critical section.
        return;

    // Need to touch cpuflushcache pages or cpuflushcache can cause
    // page faults in the middle of its cache flush.
    uint32 *p = &cpuFlushCache_data;
    while (p < &cpuFlushCache_dataend) {
        volatile uint32 x;
        x = *p++;
    }

    // Ask wince to do privilege escalation.
    SetKMode(TRUE);

    // Disable interrupts.
    unsigned long temp;
    __asm__ __volatile__(
        "mrs    %0, cpsr\n"
        "       orr    %0, %0, #0xc0\n"
        "       msr    cpsr_c, %0"
        : "=r" (temp) : : "memory");
}

void
return_control()
{
    if (--controlCount)
        // Still in the critical section.
        return;

    // Reenable interrupts.
    unsigned long temp;
    __asm__ __volatile__(
        "mrs    %0, cpsr\n"
        "       bic    %0, %0, #0xc0\n"
        "       msr    cpsr_c, %0"
        : "=r" (temp) : : "memory");

    SetKMode(FALSE);
}
