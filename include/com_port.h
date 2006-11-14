/* code by ynezz@hysteria.sk for conditions of use see file COPYING */

#ifndef _COMPORT_H
#define _COMPORT_H

#include "xtypes.h"

extern "C" int com_port_open ();
extern "C" int com_port_close ();
extern "C" int com_port_write (char *data, uint32 count);

#endif /* _COMPORT_H */
