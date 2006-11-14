/*
    CPU & coprocessor manipulations
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#if 0

#include <time.h>

#include "xtypes.h"
#include "haret.h"
#include "output.h"
#include "memory.h"

#define ICR 0x40d00000
#define GCR 0x40e00000

extern "C" {
/* Old IRQ handler */
void (*old_irq_handler) ();
/* Count primary PXA IRQs */
extern uint8 irq_count [32];
/* Count secondary PXA GPIO IRQs */
extern uint8 gpio_irq_count [84];
/* Interrupt controller registers */
uint32 *icr;
/* GPIO control registers */
uint32 *gcr;
/* The replacement IRQ handler routine */
extern void irq_chained_handler ();
extern void end_irq_chained_handler;
}

static char *irq_names [32] =
{
  "", "", "", "", "", "", "",
  "HWUART", "GPIO0", "GPIO1", "GPIO2-80", "USB",
  "PMU", "I2S", "AC97", "", "Net SSP", "LCD", "I2C",
  "ICP", "STUART", "BTUART", "FFUART", "MMC",
  "SSP", "DMA", "TMR0", "TMR1", "TMR2", "TMR3",
  "RTC0", "RTC1"
};

/*
  uint32 i, x = icr [1] & 0xfffe7f00;
  if (x & 0x400)
  {
    for (i = 0; i <= 80; i++)
      if (gcr [18 + (i >> 5)] & (1 << (i & 31)))
        irq_gpio_count [i]++;
  }
  for (i = 0; i < 32; i++, x >>= 1)
    if (x & 1)
      irq_count [i]++;
      */

static void irqWatch (uint seconds)
{
  int cur_time = time (NULL);
  int fin_time = cur_time + seconds;

  memset (&irq_count, 0, sizeof (irq_count));

  uint32 int_table = memVirtToPhys (0xffff0018);
  uint32 *x = (uint32 *)memPhysMap (int_table);
  // Check if instruction is LDR PC,#
  if ((*x & 0xfffff000) != 0xe59ff000)
  {
    Complain (L"Unknown instruction at address %08x", x);
    return;
  }

  // Map interrupt controller registers to virtual address space
  icr = (uint32 *)memPhysMap (ICR);
  gcr = (uint32 *)memPhysMap (GCR);

  uint32 *y = (uint32 *)(uint32 (x) + 8 + (*x & 0xfff));
  old_irq_handler = (void (*) ())(*y);

  Output (L"Collecting data, please wait ...");

  memset (&irq_count, 0, sizeof (irq_count));
  memset (&irq_gpio_count, 0, sizeof (irq_gpio_count));

  *y = (uint32)&irq_chained_handler;

  while (cur_time <= fin_time)
    cur_time = time (NULL);

  *y = (uint32)old_irq_handler;

  Output (L"done. Summary on IRQs:");
  int i, j;
  for (i = 0; i < 32; i++)
    if (irq_count [i])
    {
      Output (L"IRQ %d (%s) occured %d times", i, irq_names [i], irq_count [i]);
      if (i == 10)
        for (j = 0; j <= 80; j++)
          if (irq_gpio_count [j])
            Output (L"  IRQ from GPIO%d occured %d times", j, irq_gpio_count [j]);
    }
}

static void
cmd_wirq(const char *cmd, const char *args)
{
    uint32 sec;
    if (!get_expression (&x, &sec))
    {
        Complain (C_ERROR ("line %d: Expected <seconds>"), line);
        return;
    }
    irqWatch (sec);
}
REG_CMD(0, "WI|RQ", cmd_wirq,
        "WIRQ <seconds>\n"
        "  Watch which IRQ occurs for some period of time and report them.")

#endif
