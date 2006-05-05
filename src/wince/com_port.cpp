/* code by ynezz@hysteria.sk for conditions of use see file COPYING */
#include <windows.h>
#include <stdio.h>
#include "com_port.h"

static HANDLE port_handle = INVALID_HANDLE_VALUE;

int com_port_open ()
{
  DCB port_dcb;
  COMMTIMEOUTS port_timeouts;

  port_handle = CreateFile (L"COM1:", GENERIC_READ | GENERIC_WRITE,
    0, NULL, OPEN_EXISTING, 0, NULL);

  if (port_handle == INVALID_HANDLE_VALUE)
    return FALSE;

  if (!GetCommState (port_handle, &port_dcb))
  {
    com_port_close ();
    return FALSE;
  }

  port_dcb.BaudRate = 115200;
  port_dcb.ByteSize = 8;
  port_dcb.Parity = NOPARITY;
  port_dcb.StopBits = ONESTOPBIT;

  if (SetCommState (port_handle, &port_dcb) == 0)
  {
    com_port_close ();
    return FALSE;
  }

  if (SetCommTimeouts (port_handle, &port_timeouts) == 0)
  {
    com_port_close ();
    return FALSE;
  }

  return TRUE;

}

int com_port_close ()
{
  if (port_handle == INVALID_HANDLE_VALUE)
    return FALSE;

  CloseHandle (port_handle);

  return TRUE;
}

int com_port_write (char *data, unsigned count)
{
  DWORD data_written;

  if (!WriteFile (port_handle, data, count, &data_written, NULL))
    return FALSE;

  if (count != data_written)
    return FALSE;

  return TRUE;
}
