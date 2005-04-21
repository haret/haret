/*
    Poor Man's Hardware Reverse Engineering Tool
    Network connection handling
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "xtypes.h"
#include "cpu.h"
#include "util.h"
#include "output.h"
#include "terminal.h"
#include "script.h"

#ifdef _WIN32_WCE
#  include <winsock.h>
#  define so_close	closesocket
#  define so_ioctl	ioctlsocket

static struct __network_dummy
{
  __network_dummy ()
  {
    // Initialize sockets
    WSADATA wsadata;
    WSAStartup (MAKEWORD(2, 0), &wsadata);
  }
  ~__network_dummy ()
  {
    WSACleanup ();
  }
} __network_dummy_obj;

#else
#  include <sys/socket.h>
#  include <netinet/in.h>
#  include <arpa/inet.h>
#  include <netdb.h>
#  include <unistd.h>
#  define so_close	close
#  define so_ioctl	ioctl
#endif

static int sock;

// Our private haretTerminal extension that reads/writes to socket
class haretNetworkTerminal : public haretTerminal
{
  int socket;

private:
  virtual int Read (uchar *indata, size_t max_len);
  virtual int Write (const uchar *outdata, size_t len);

public:
  haretNetworkTerminal (int iSocket) : haretTerminal ()
  { socket = iSocket; }
};

int haretNetworkTerminal::Read (uchar *indata, size_t max_len)
{
  if (!max_len)
    return 0;

  // Will work in char-by-char mode to avoid fiddling with non-blocking
  // sockets et al (which I'm not sure works POSIXly in WinCE).
  return recv (socket, (char *)indata, 1, 0);
}

int haretNetworkTerminal::Write (const uchar *outdata, size_t len)
{
  return send (socket, (char *)outdata, len, 0);
}

static void sock_output (wchar_t *msg, wchar_t *title)
{
  int len = 0;
  char sbcs [300];
  wchar_t *str [2] = { title, msg };
  for (int i = 0; i < 2; i++)
  {
    if (!str [i])
      continue;

#ifdef _WIN32_WCE
    BOOL flag;
    len += WideCharToMultiByte (CP_ACP, 0, str [i], wstrlen (str [i]),
                                sbcs + len, sizeof (sbcs) - len, " ", &flag);
    while ((len > 0) && sbcs [len - 1] == 0)
      len--;
#else
    for (int j = 0; str [i] [j]; j++)
      sbcs [len++] = str [i] [j];
#endif
    // If there is a caption, add ": " after it
    if (i == 0)
    {
      sbcs [len++] = ':';
      sbcs [len++] = ' ';
    }
  }

  if (sbcs [len - 1] == '\t')
    len--;
  else
    sbcs [len++] = '\n';

  // Convert CR to CR/LF since telnet requires this
  char *dst = sbcs + sizeof (sbcs) - 1;
  char *eol = sbcs + len - 1;
  while (eol >= sbcs)
  {
    if ((*dst-- = *eol) == '\n')
      *dst-- = '\r';
    eol--;
  }

  dst++;
  len = sbcs + sizeof (sbcs) - dst;
  send (sock, dst, len, 0);
}

static char *cpu_id (uint p15r0)
{
  static char buff [100];
  int top = 0;

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

static const char *cpu_mode (uint mode)
{
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

void scrListen (int port)
{
  int lsock = socket (AF_INET, SOCK_STREAM, 0);
  if (lsock < 0)
  {
    Complain (C_ERROR ("Failed to create socket"));
    return;
  }

  struct sockaddr_in addr;
  memset (&addr, 0, sizeof (addr));
  addr.sin_family = AF_INET;
  addr.sin_port = htons (port);
  addr.sin_addr.s_addr = inet_addr ("0.0.0.0");

  if (bind (lsock, (struct sockaddr *)&addr, sizeof (addr)) < 0)
  {
    Complain (C_ERROR ("Failed to bind socket"));
    goto finish;
  }

  Status (L"Waiting for connection ...");

  if (listen (lsock, 1) < 0)
    goto conn_error;

#ifdef _WIN32_WCE
  ShowCursor (TRUE);
  HCURSOR OldCursor;
  OldCursor = GetCursor ();
  SetCursor (LoadCursor (NULL, IDC_WAIT));
#endif

  int addrlen;
  addrlen = sizeof (addr);
  sock = accept (lsock, (struct sockaddr *)&addr, &addrlen);

#ifdef _WIN32_WCE
  if (OldCursor != (HCURSOR)-1)
  {
    SetCursor (OldCursor);
    OldCursor = (HCURSOR)-1;
  }
#endif

  if (sock < 0)
  {
conn_error:
    Complain (C_ERROR ("Connection failed"));
    goto finish;
  }

  Status (L"Connect from %hs:%d", inet_ntoa (addr.sin_addr),
          htons (addr.sin_port));
  Log (L"Incoming connection from %hs:%d", inet_ntoa (addr.sin_addr),
       htons (addr.sin_port));

  // Close the gate
  so_close (lsock);
  lsock = -1;

  output_fn = sock_output;

  // Display some welcome message
#ifdef _WIN32_WCE
  SYSTEM_INFO si;
  GetSystemInfo (&si);
  OSVERSIONINFOW vi;
  vi.dwOSVersionInfoSize = sizeof (vi);
  GetVersionEx (&vi);
  Output (L"Welcome, this is HaRET running on WindowsCE v%d.%d\n"
          L"Minimal virtual address: %08x, maximal virtual address: %08x",
          vi.dwMajorVersion, vi.dwMinorVersion,
          si.lpMinimumApplicationAddress, si.lpMaximumApplicationAddress);
#endif
  Output (L"CPU is %hs running in %hs mode\n"
          L"Enter 'HELP' for a short command summary.\n",
	  cpu_id (cpuGetCP (15, 0)), cpu_mode (cpuGetPSR () & 0x1f));

  {
    haretNetworkTerminal t (sock);
    if (t.Initialize ())
      for (int line = 1; ; line++)
      {
        // Some kind of prompt
	wchar_t prompt [16];
	_snwprintf (prompt, sizeof (prompt) / sizeof (wchar_t), 
	            L"HaRET(%d)# ", line);

        // Convert to SBCS (simple because we don't have non-ascii chars)
	int i;
        char sbcsprompt [10];
	for (i = 0; prompt [i]; i++)
	  sbcsprompt [i] = (char)prompt [i];
	sbcsprompt [i] = 0;

        if (!t.Readline (sbcsprompt))
          break;

        if (!scrInterpret ((char *)t.GetStr (), line))
          break;
      }
  }

  output_fn = NULL;
  so_close (sock);

  Log (L"Connection from %hs:%d terminated", inet_ntoa (addr.sin_addr),
       htons (addr.sin_port));

finish:
  if (lsock != -1)
    so_close (lsock);

  Status (L"");
}
