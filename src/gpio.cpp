/*
    Periferial I/O devices interface
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <time.h>

#include "xtypes.h"
#include "gpio.h"
#include "memory.h"
#include "output.h"
#include "script.h" // REG_VAR_BITSET
#include "arch-pxa.h" // testPXA

// Which GPIO changes to ignore during watch
uint32 gpioIgnore [3] = { 0, 0, 0 };

REG_VAR_BITSET(testPXA, "IGPIO", gpioIgnore, 84
               , "The list of GPIOs to ignore during WGPIO")

void gpioSetDir (int num, bool out)
{
  if (num > 84)
    return;

  uint32 *gpdr = (uint32 *)memPhysMap (GPDR0);
  uint32 ofs = num >> 5;
  uint32 mask = 1 << (num & 31);

  if (out)
    gpdr [ofs] |= mask;
  else
    gpdr [ofs] &= ~mask;
}

bool gpioGetDir (int num)
{
  if (num > 84)
    return false;

  uint32 *gpdr = (uint32 *)memPhysMap (GPDR0);

  return (gpdr [num >> 5] & (1 << (num & 31))) != 0;
}

void gpioSetAlt (int num, int altfn)
{
  if (num > 84)
    return;

  uint32 *gafr = (uint32 *)memPhysMap (GAFR0_L);
  uint32 ofs = num >> 4;
  uint32 shft = (num & 15) << 1;

  gafr [ofs] = (gafr [ofs] & ~(3 << shft)) | ((altfn & 3) << shft);
}

uint32 gpioGetAlt (int num)
{
  if (num > 84)
    return -1;

  uint32 *gafr = (uint32 *)memPhysMap (GAFR0_L);

  return (gafr [num >> 4] >> ((num & 15) << 1)) & 3;
}

int gpioGetState (int num)
{
  if (num > 84)
    return -1;

  uint32 *gplr = (uint32 *)memPhysMap (GPLR0);
  return (gplr [num >> 5] >> (num & 31)) & 1;
}

void gpioSetState (int num, bool state)
{
  if (num > 84)
    return;

  uint32 *gpscr = (uint32 *)memPhysMap (state ? GPSR0 : GPCR0);
  gpscr [num >> 5] |= 1 << (num & 31);
}

int gpioGetSleepState (int num)
{
  if (num > 84)
    return -1;

  uint32 *pgsr = (uint32 *)memPhysMap (PGSR0);
  return (pgsr [num >> 5] >> (num & 31)) & 1;
}

void gpioSetSleepState (int num, bool state)
{
  if (num > 84)
    return;

  uint32 *pgsr = (uint32 *)memPhysMap (PGSR0);
  uint32 ofs = num >> 5;
  uint32 mask = 1 << (num & 31);

  if (state)
    pgsr [ofs] |= mask;
  else
    pgsr [ofs] &= ~mask;
}

// Watch for given number of seconds which GPIO pins change
static void gpioWatch (uint seconds)
{
  if (seconds > 60)
  {
    Output(C_INFO "Number of seconds trimmed to 60");
    seconds = 60;
  }

  int cur_time = time (NULL);
  int fin_time = cur_time + seconds;
  int i, j;

  uint32 *gplr = (uint32 *)memPhysMap (GPLR0);
  uint32 old_gplr [3];
  for (i = 0; i < 3; i++)
    old_gplr [i] = gplr [i];

  uint32 *gpdr = (uint32 *)memPhysMap (GPDR0);
  uint32 old_gpdr [3];
  for (i = 0; i < 3; i++)
    old_gpdr [i] = gpdr [i];

  uint32 *gafr = (uint32 *)memPhysMap (GAFR0_L);
  uint32 old_gafr [6];
  for (i = 0; i < 6; i++)
    old_gafr [i] = gafr [i];

  while (cur_time <= fin_time)
  {
    gplr = (uint32 *)memPhysMap (GPLR0);
    for (i = 0; i < 3; i++)
    {
      uint32 val = gplr [i];
      if (old_gplr [i] != val)
      {
        uint32 changes = (old_gplr [i] ^ val) & ~gpioIgnore [i];
        for (j = 0; j < 32; j++)
          if ((changes & (1 << j))
	   && ((i * 32 + j) <= 84))
	  {
            Screen("GPLR[%d] changed to %d", i * 32 + j, (val >> j) & 1);
          }
        old_gplr [i] = val;
      }
    }

    gpdr = (uint32 *)memPhysMap (GPDR0);
    for (i = 0; i < 3; i++)
    {
      uint32 val = gpdr [i];
      if (old_gpdr [i] != val)
      {
        uint32 changes = (old_gpdr [i] ^ val) & ~gpioIgnore [i];
        for (j = 0; j < 32; j++)
          if ((changes & (1 << j))
	   && ((i * 32 + j) <= 84))
	  {
            Screen("GPDR[%d] changed to %d", i * 32 + j, (val >> j) & 1);
	  }
        old_gpdr [i] = val;
      }
    }

    gafr = (uint32 *)memPhysMap (GAFR0_L);
    for (i = 0; i < 6; i++)
    {
      uint32 val = gafr [i];
      if (old_gafr [i] != val)
      {
        uint32 changes = old_gafr [i] ^ val;
        for (j = 0; j < 16; j++)
          if (changes & (3 << j * 2)
	   && ((i * 32 + j) <= 83))
	  {
            Screen("GAFR[%d] changed to %d", i * 16 + j, (changes >> j * 2) & 3);
	  }
        old_gafr [i] = val;
      }
    }

    cur_time = time (NULL);
  }
}

static void
cmd_wgpio(const char *cmd, const char *x)
{
    uint32 sec;
    if (!get_expression(&x, &sec)) {
        ScriptError("Expected <seconds>");
        return;
    }
    gpioWatch(sec);
}
REG_CMD(testPXA, "WG|PIO", cmd_wgpio,
        "WGPIO <seconds>\n"
        "  Watch GPIO pins for given period of time and report changes.")

static uint32 gpioScrGPLR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > 84)
  {
    Output("Valid GPIO indexes are 0..84, not %d", args [0]);
    return -1;
  }

  if (setval)
  {
    gpioSetState (args [0], val != 0);
    return 0;
  }

  return gpioGetState (args [0]);
}
REG_VAR_RWFUNC(testPXA, "GPLR", gpioScrGPLR, 1,
               "General Purpose I/O Level Register")

static uint32 gpioScrGPDR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > 84)
  {
    Output("Valid GPIO indexes are 0..84, not %d", args [0]);
    return -1;
  }

  if (setval)
  {
    gpioSetDir (args [0], val != 0);
    return 0;
  }

  return gpioGetDir (args [0]);
}
REG_VAR_RWFUNC(testPXA, "GPDR", gpioScrGPDR, 1
               , "General Purpose I/O Direction Register")

static uint32 gpioScrGAFR (bool setval, uint32 *args, uint32 val)
{
  if (args [0] > 84)
  {
    Output("Valid GPIO indexes are 0..84, not %d", args [0]);
    return -1;
  }

  if (setval)
  {
    gpioSetAlt (args [0], val);
    return 0;
  }

  return gpioGetAlt (args [0]);
}
REG_VAR_RWFUNC(testPXA, "GAFR", gpioScrGAFR, 1
               , "General Purpose I/O Alternate Function Select Register")

// Dump the overall GPIO state
static void
gpioDump(const char *tok, const char *args)
{
  const uint rows = 84/4;
  uint32 *grer = (uint32 *)memPhysMap (GRER0);
  uint32 *gfer = (uint32 *)memPhysMap (GFER0);

  Output("GPIO# D S A INTER | GPIO# D S A INTER | GPIO# D S A INTER | GPIO# D S A INTER");
  Output("------------------+-------------------+-------------------+------------------");
  for (uint i = 0; i < rows; i++)
  {
    for (uint j = 0; j < 4; j++)
    {
      uint gpio = i + j * rows;
      bool re = (grer [gpio >> 5] & (1 << (gpio & 31))) != 0;
      bool fe = (gfer [gpio >> 5] & (1 << (gpio & 31))) != 0;

      Output("%3d   %c %d %d %s%s%s", gpio, gpioGetDir (gpio) ? 'O' : 'I',
           gpioGetState (gpio), gpioGetAlt (gpio),
           re ? "RE " : "   ", fe ? "FE" : "  ",
           j < 3 ? " | \t" : "");
    }
  }
}
REG_DUMP(testPXA, "GPIO", gpioDump,
         "GPIO\n"
         "  Show GPIO machinery state in a human-readable format.")

// Dump GPIO state in a linux-specific format
static void
gpioDumpState(const char *tok, const char *args)
{
  int i;
  Output("/* GPIO pin direction setup */");
  for (i = 0; i < 81; i++)
    Output("#define GPIO%02d_Dir\t%d",
         i, gpioGetDir (i));
  Output("\n/* GPIO Alternate Function (Select Function 0 ~ 3) */");
  for (i = 0; i < 81; i++)
    Output("#define GPIO%02d_AltFunc\t%d",
         i, gpioGetAlt (i));
  Output("\n/* GPIO Pin Init State */");
  for (i = 0; i < 81; i++)
    Output("#define GPIO%02d_Level\t%d",
         i, gpioGetDir (i) ? gpioGetState (i) : 0);
  Output("\n/* GPIO Pin Sleep Level */");
  for (i = 0; i < 81; i++)
    Output("#define GPIO%02d_Sleep_Level\t%d",
         i, gpioGetSleepState (i));
}
REG_DUMP(testPXA, "GPIOST", gpioDumpState,
         "GPIOST\n"
         "  Show GPIO state suitable for include/asm/arch/xxx-init.h")
