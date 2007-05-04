/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "windows.h"
#include <stdio.h> // sprintf

#include "xtypes.h"
#include "output.h" // Output
#include "machines.h" // Mach
#include "script.h" // REG_VAR_ROFUNC
#include "cpu.h"

DEF_GETCPR(get_p15r0, p15, 0, c0, c0, 0)

static char *cpu_id()
{
  static char buff [100];
  int top = 0;

  uint p15r0 = get_p15r0();

#define PUTS(s) \
  { size_t sl = strlen (s); memcpy (buff + top, s, sl); top += sl; }
#define PUTSF(s, n) \
  { sprintf (buff + top, s, n); top += strlen (buff + top); }

  // First the vendor
  switch (p15r0 >> 24)
  {
    case 'A': PUTS ("ARM "); break;
    case 'D': PUTS ("DEC "); break;
    case 'i': PUTS ("Intel "); break;
    default:  PUTS ("Unknown "); break;
  }

  if ((p15r0 >> 24) == 'i'
   && ((p15r0 >> 13) & 7) == 1)
    PUTS ("XScale ");

  PUTS ("ARM arch ");

  switch ((p15r0 >> 16) & 15)
  {
    case 1: PUTS ("4 "); break;
    case 2: PUTS ("4T "); break;
    case 3: PUTS ("5 "); break;
    case 4: PUTS ("5T "); break;
    case 5: PUTS ("5TE "); break;
    default: PUTSF ("unknown(%d) ", (p15r0 >> 16) & 15); break;
  }

  if ((p15r0 >> 24) == 'i')
  {
    PUTSF ("revision %d ", (p15r0 >> 10) & 7);
    PUTSF ("product %d ", (p15r0 >> 4) & 63);
  }

  PUTSF ("stepping %d", p15r0 & 15);

  buff [top] = 0;
  return buff;
}

static const char *cpu_mode()
{
  uint mode = cpuGetPSR() & 0x1f;

  switch (mode)
  {
    case 0x10:
      return "user";
    case 0x11:
      return "FIQ";
    case 0x12:
      return "IRQ";
    case 0x13:
      return "supervisor";
    case 0x17:
      return "abort";
    case 0x1b:
      return "undefined";
    case 0x1f:
      return "system";
    default:
      return "unknown";
  }
}

// Print out info on haret, the machine, and wince.
void
printWelcome()
{
    // Display some welcome message
    SYSTEM_INFO si;
    GetSystemInfo (&si);
    OSVERSIONINFOW vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    GetVersionEx(&vi);

    WCHAR bufplat[128], bufoem[128];
    SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(bufplat),&bufplat, 0);
    SystemParametersInfo(SPI_GETOEMINFO, sizeof(bufoem),&bufoem, 0);

    Output("Welcome, this is HaRET %s running on WindowsCE v%ld.%ld\n"
           "Minimal virtual address: %p, maximal virtual address: %p",
           VERSION, vi.dwMajorVersion, vi.dwMinorVersion,
           si.lpMinimumApplicationAddress, si.lpMaximumApplicationAddress);
    Output("Detected machine %s/%s (Plat='%ls' OEM='%ls')\n"
           "CPU is %s running in %s mode\n"
           "Enter 'HELP' for a short command summary.\n",
           Mach->name, Mach->archname, bufplat, bufoem,
           cpu_id(), cpu_mode());
}

uint32 cmd_cpuGetPSR(bool, uint32*, uint32) {
    return cpuGetPSR();
}
REG_VAR_ROFUNC(0, "PSR", cmd_cpuGetPSR, 0, "Program Status Register")

DEF_GETCPR(get_p15r2, p15, 0, c2, c0, 0)

// Returns the address of 1st level descriptor table
uint32 cpuGetMMU()
{
    return get_p15r2() & 0xffffc000;
}
uint32 cmd_cpuGetMMU(bool, uint32*, uint32) {
    return cpuGetMMU();
}
REG_VAR_ROFUNC(
    0, "MMU", cmd_cpuGetMMU, 0,
    "Memory Management Unit level 1 descriptor table physical addr")

DEF_GETCPR(get_p15r13, p15, 0, c13, c0, 0)

// Returns the PID register contents
static uint32 cpuGetPID(bool, uint32*, uint32)
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
        uint32 dummy;
        dummy = *(volatile uint32*)start;
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
static void
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

// Return to normal processing.
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
