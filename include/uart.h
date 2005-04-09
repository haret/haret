/*
	UART driver 
	Copyright (c) 2005 Ben Dooks

    For conditions of use see file COPYING

	$Id: uart.h,v 1.1 2005/04/09 17:00:37 fluffy Exp $
*/


struct uart_drv {
	void	(*setup)(void);
	void	(*puts)(char *ch);
};

extern void UART_puts(char *s);

extern void UART_setup(void);

extern void UART_setDriver(struct uart_drv *drv);

