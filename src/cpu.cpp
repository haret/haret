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
cpuDumpAC97(uint32 *args)
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
      Output("Register %x: codec is busy", i * 2);
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
      Output("Register %x: access timed out", i * 2);
      continue;
    }

    regs [i] = *reg;

    return_control();

    // Shit, if we remove this it won't work correctly :-(
    Output(".\b");
  }
  Output("\n");

  ac97->_POCR = old_pocr;
  ac97->_PICR = old_picr;
  ac97->_MCCR = old_mccr;
  ac97->_MOCR = old_mocr;
  ac97->_MICR = old_micr;
  ac97->_GCR = old_gcr;

  Output("GCR:  %08x  MCCR: %08x", old_gcr,  old_mccr);
  Output("POCR: %08x  PICR: %08x", old_pocr, old_picr);
  Output("MOCR: %08x  MICR: %08x", old_mocr, old_micr);

  for (i = 0; i < 16; i++)
    Output("r%02x: %04x | r%02x: %04x | r%02x: %04x | r%02x: %04x",
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
static uint32 cpuGetPID()
{
    return get_p15r13() >> 25;
}
REG_VAR_ROFUNC(0, "PID", cpuGetPID, 0,
               "Current Process Identifier register value")

// Symbols added by linker.
extern "C" {
    extern uint32 _text_start;
    extern uint32 _text_end;
    extern uint32 _data_start;
    extern uint32 _data_end;
    extern uint32 _rdata_start;
    extern uint32 _rdata_end;
    extern uint32 _bss_start;
    extern uint32 _bss_end;
}

// Access all the pages in a pointer range.  (This forces wince to
// make sure the page is mapped.)
static void
touchPages(uint32 *start, uint32 *end)
{
    if (PAGE_ALIGN((uint32)start) != (uint32)start)
        Output("Internal error. touchPages range not page aligned");

    while (start < end) {
        volatile uint32 dummy;
        dummy = *start;
        start += (PAGE_SIZE/sizeof(*start));
    }
}

// Touch all the code pages of the HaRET application.
//
// wm5 has been seen lazily mapping in code pages.  That is, it may
// not actually load certain functions (or parts of a function) into
// memory until they are actually used.  This presents problems for
// certain haret functions that try to take full control of the CPU,
// because part of the code could might not yet be mapped.  When this
// code is accessed it causes a fault that hands control back to wm5.
// A solution is to touch all code pages to ensure the code is really
// in memory.
void
touchAppPages(void)
{
    touchPages(&_text_start, &_text_end);
    touchPages(&_data_start, &_data_end);
    touchPages(&_rdata_start, &_rdata_end);
    touchPages(&_bss_start, &_bss_end);
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

    // Flush log to disk (in case we don't survive)
    flushLogFile();

    // Map in pages to prevent page faults in critical section.
    touchAppPages();

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
}
