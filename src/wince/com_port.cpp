/* code by ynezz@hysteria.sk for conditions of use see file COPYING */
#include <windows.h>
#include <stdio.h>
#include "xtypes.h"
#include "com_port.h"
#include "output.h"
#include "script.h" // REG_VAR_RWFUNC

static HANDLE port_handle = INVALID_HANDLE_VALUE;
static uint32 port_number = 0;

static uint32 comScrNumber (bool setval, uint32 *args, uint32 val)
{
  if (setval)
  {
    if (val > 0 && val < 100)
    {
      port_number = val;
      if (com_port_open())
      {
        char w [50];
        sprintf (w, "Comport init COM%i:115200,8N1", port_number);
        com_port_write (w, strlen (w) - 1);
      }
      return 0;
    }
  }
  return port_number;
}
REG_VAR_RWFUNC(
    0, "COM", comScrNumber, 0
    , "COM port number initialized to 115200,8N1 before booting linux")

int com_port_open ()
{
  DCB port_dcb;
  COMMTIMEOUTS port_timeouts;
  wchar_t port[6];
  
  if (port_number > 0 && port_number < 100)
    wsprintf (port, L"COM%i:", port_number);
  else
    return FALSE;

  port_handle = CreateFile (port, GENERIC_READ | GENERIC_WRITE,
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

int com_port_write (char *data, uint32 count)
{
  uint32 data_written;

  if (port_handle == INVALID_HANDLE_VALUE)
    return FALSE;

  if (!WriteFile (port_handle, data, count, (LPDWORD)&data_written, NULL))
    return FALSE;

  if (count != data_written)
    return FALSE;

  return TRUE;
}
