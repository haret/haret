/*
    Linux loader for Windows CE
    Copyright (C) 2005 Ben Dooks

    For conditions of use see file COPYING

    $Id: cpu-s3c24xx.cpp,v 1.8 2006/11/19 23:37:14 koconnor Exp $
*/


#include "haret.h"
#include "xtypes.h"
#define CONFIG_ACCEPT_GPL
#include "setup.h"
#include "memory.h"
#include "output.h"
#include "gpio.h"
#include "video.h"
#include "cpu.h"
#include "uart.h"
#include "resource.h"
#include "s3c24xx.h"

static uint32 *s3c_gpio;

static inline uint32 s3c_readl(volatile uint32 *base, uint32 reg)
{
  return base [reg/4];
}

static inline void s3c_writel(volatile uint32 *base, uint32 reg, uint32 val)
{
  base [(reg/4)] = val;
}

// uart drivers

#define S3C_UART (0x50008000)

//#define uart_full(base) (((base)[0x10/4] & (1<<2)) == 0)

/* general s3c24xx uart support */

static volatile uint32 *uart_base;

static void UART_s3c24xx_map(void)
{
  if (uart_base == NULL)
  {
    uart_base = (volatile uint32 *)VirtualAlloc ((void*)0x0, sizeof (void*) * 0xffff,
      MEM_RESERVE, PAGE_READWRITE);
    VirtualCopy ((void *)uart_base, (void *) ((S3C_UART)/256), sizeof(void *) * 0xffff,
      PAGE_READWRITE | PAGE_NOCACHE | PAGE_PHYSICAL);
  }
}

/* S3C2440 */

static void UART_s3c2440_setup()
{
  UART_s3c24xx_map();

  uart_base[0x00] = 0x43;
  uart_base[0x01] = 0x8c05;
  uart_base[0x02] = 0x06;
  uart_base[0x03] = 0x0;
  uart_base[0x28/4] = 0x120;

  //UART_s3c24xx_puts("testing UART...\n\r");
}

static void UART_s3c2440_checksetup(void)
{
  if (uart_base[0x00] != 0x43)
    uart_base[0x00] = 0x43;
  if (uart_base[0x01] != 0x8c05)
    uart_base[0x01] = 0x8c05;
  if (uart_base[0x02] & 0x01)
    uart_base[0x02] = 0x06;
  if (uart_base[0x03] != 0)
    uart_base[0x03] = 0;
}

static int UART_s3c2440_full(volatile uint32 *base)
{
  if (s3c_readl(base, S3C2410_UFCON) & S3C2410_UFCON_FIFOMODE)
    return s3c_readl(base, S3C2410_UFSTAT) & S3C2440_UFSTAT_TXFULL;

  return !(s3c_readl(base, S3C2410_UTRSTAT) & S3C2410_UTRSTAT_TXFE);
}

static void UART_s3c2440_putc(char c)
{
  if (uart_base == NULL)
    UART_s3c2440_setup ();

  UART_s3c2440_checksetup ();

  while(UART_s3c2440_full(uart_base))
    UART_s3c2440_checksetup ();

  ((volatile u8 *)uart_base)[S3C2410_UTXH] = c;
}

static void UART_s3c2440_puts (char *s)
{
  int a = 0;

  while (s [a])
  {
    UART_s3c2440_putc (s [a]);
    a++;
  }
}

static struct uart_drv s3c2440_uarts =
{
  UART_s3c2440_setup,
  UART_s3c2440_puts
};

/* S3C2410 UART driver */

static int UART_s3c2410_full (volatile uint32 *base)
{
  if (s3c_readl(base, S3C2410_UFCON) & S3C2410_UFCON_FIFOMODE)
    return s3c_readl (base, S3C2410_UFSTAT) & S3C2410_UFSTAT_TXFULL;

  return !(s3c_readl (base, S3C2410_UTRSTAT) & S3C2410_UTRSTAT_TXFE);
}

static void UART_s3c2410_checksetup(void)
{
}

static void UART_s3c2410_setup (void)
{
  UART_s3c24xx_map();
}

static void UART_s3c2410_putc (char c)
{
  if (uart_base == NULL)
    UART_s3c2410_setup ();

  UART_s3c2410_checksetup ();

  while(UART_s3c2410_full(uart_base))
    UART_s3c2410_checksetup ();

  ((volatile u8 *)uart_base) [S3C2410_UTXH] = c;
}

static void UART_s3c2410_puts (char *s)
{
  int a = 0;

  while (s [a])
  {
    UART_s3c2440_putc (s [a]);
    a++;
  }
}

static bool s3c24xxDetect ()
{
  uint32 p15r0 = cpuGetCP (15, 0);

  // please add proper detection for Samsung CPUs
#if 0
  if ((p15r0 >> 24) == 'i'
   && ((p15r0 >> 13) & 7) == 1)
    return true;
#endif

  return false;
}

static struct uart_drv s3c2410_uarts =
{
  UART_s3c2410_setup,
  UART_s3c2410_puts
};


static int s3c24xxSetupLoad (void)
{
  uint32 s3c_ver;

  s3c_gpio = (uint32 *)memPhysMap(S3C2410_PA_GPIO);

  s3c_ver = s3c_readl(s3c_gpio, S3C2410_GSTATUS1);

  switch (s3c_ver & S3C2410_GSTATUS1_IDMASK)
  {
    case S3C2410_GSTATUS1_2410:
      UART_setDriver(&s3c2410_uarts);
      Output(L"Detected S3C2410, Version %d\n", s3c_ver & ~S3C2410_GSTATUS1_IDMASK);
      break;

    case S3C2410_GSTATUS1_2440:
      UART_setDriver(&s3c2440_uarts);
      Output(L"Detected S3C2440, Version %d\n", s3c_ver & ~S3C2410_GSTATUS1_IDMASK);
      break;
  
    default:
      Output(L"Unknown S3C24XX, ID 0x%08x\n", s3c_ver);
  }

  return 0;
}

// TODO - sort out shutting down IIS, DMA, etc

static void s3c24xxShutdownIIS (void)
{
}

static void s3c24xxShutdownDMA (void)
{
  int dma_ch;
  volatile uint32 *channels;
  volatile uint32 *ch;
  uint32 dmasktrig;
  int timeo;

  channels = (volatile uint32 *)memPhysMap (S3C2410_PA_DMA);
  if (channels == NULL)
    return;

  for (dma_ch = 0; dma_ch < 4; dma_ch++)
  {
    ch = channels + ((0x40 / 4) * dma_ch);
  
    dmasktrig = s3c_readl(ch, S3C2410_DMA_DMASKTRIG);
    if (dmasktrig & S3C2410_DMASKTRIG_ON)
    {
      Output(L"DMA[%d] - shutting down active DMA\n", dma_ch);

      s3c_writel(ch, S3C2410_DMA_DMASKTRIG, S3C2410_DMASKTRIG_STOP);

      timeo = 0x100000;
      while (timeo > 0)
      {
        dmasktrig = s3c_readl(ch, S3C2410_DMA_DMASKTRIG);

        if ((dmasktrig & S3C2410_DMASKTRIG_ON) == 0)
          break;

        if (timeo-- <= 0)
          Output(L"DMA[%d] - timeout waiting to stop\n", dma_ch);
      }
    }
  }
}

static int s3c24xxShutdownPerihperals(void)
{
  s3c24xxShutdownIIS ();
  s3c24xxShutdownDMA ();

  return 0;
}

static int s3c24xxAttemptRecovery(void)
{
  return 0;
}

struct cpu_fns cpu_s3c24xx =
{
  L"S3C24XX",
  s3c24xxDetect,
  s3c24xxSetupLoad,
  s3c24xxShutdownPerihperals,
  s3c24xxAttemptRecovery
};
