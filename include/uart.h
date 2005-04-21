/*
    UART driver 
    Copyright (c) 2005 Ben Dooks

    For conditions of use see file COPYING

    $Id: uart.h,v 1.2 2005/04/21 21:06:14 zap Exp $
*/

#ifndef _UART_H
#define _UART_H

struct uart_drv
{
  void  (*setup) (void);
  void  (*puts) (char *ch);
};

extern void UART_puts(char *s);
extern void UART_setup(void);
extern void UART_setDriver(struct uart_drv *drv);

#endif // _UART_H
