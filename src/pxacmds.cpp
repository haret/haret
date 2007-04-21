/*
    Commands specific to the PXA processor
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include "memory.h" // memPhysMap
#include "pxa2xx.h" // pxaAC97
#include "output.h" // Output
#include "script.h" // REG_DUMP
#include "cpu.h" // take_control
#include "arch-pxa.h" // testPXA

static void
cpuDumpAC97(const char *tok, const char *args)
{
    uint32 unit;
    if (!get_expression(&args, &unit)) {
        ScriptError("Expected <id>");
        return;
    }

  if (unit > 3)
  {
    Output(C_ERROR "AC97 unit number must be between 0 or 3");
    return;
  }

  pxaAC97 volatile *ac97 = (pxaAC97 *)memPhysMap (AC97_BASE);
  if (!ac97)
  {
    Output(C_ERROR "Cannot map AC97 controller's physical memory");
    return;
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
}
REG_DUMP(testPXA, "AC97", cpuDumpAC97,
         "AC97(id)\n"
         "  Dump AC97 registers (id = ctrl number, 0..3).")
