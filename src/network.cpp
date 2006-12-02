/*
    Poor Man's Hardware Reverse Engineering Tool
    Network connection handling
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <stdio.h> // sprintf
#include <windows.h>
#include "pkfuncs.h" // SetKMode

#include "xtypes.h"
#include "cpu.h" // cpuGetPSR
#include "output.h" // Output, setOuptutFn
#include "terminal.h" // haretNetworkTerminal
#include "script.h" // scrInterpret
#include "machines.h" // Mach
#include "network.h"

#  include <winsock.h>
#  define so_close	closesocket
#  define so_ioctl	ioctlsocket

// Our private haretTerminal extension that reads/writes to socket
class haretNetworkTerminal : public haretTerminal, public outputfn
{
  int socket;

private:
  virtual int Read (uchar *indata, size_t max_len);
  virtual int Write (const uchar *outdata, size_t len);
  void sendMessage(const char *msg);

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

void
haretNetworkTerminal::sendMessage(const char *msg)
{
    uint len = strlen(msg);
    char sbcs[300];
    if (len > sizeof(sbcs)-10)
        len = sizeof(sbcs)-10;
    memcpy(sbcs, msg, len);

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
    send(socket, dst, len, 0);
}

DEF_GETCPR(get_p15r0, p15, 0, c0, c0, 0)

static char *cpu_id()
{
  static char buff [100];
  int top = 0;

  uint p15r0 = get_p15r0();

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

static const char *cpu_mode()
{
  uint mode = cpuGetPSR() & 0x1f;

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

static void
mainnetloop(int sock)
{
    haretNetworkTerminal t(sock);
    setOutputFn(&t);

    // Display some welcome message
    SYSTEM_INFO si;
    GetSystemInfo (&si);
    OSVERSIONINFOW vi;
    vi.dwOSVersionInfoSize = sizeof(vi);
    GetVersionEx(&vi);

    WCHAR bufplat[128], bufoem[128];
    SystemParametersInfo(SPI_GETPLATFORMTYPE, sizeof(bufplat),&bufplat, 0);
    SystemParametersInfo(SPI_GETOEMINFO, sizeof(bufoem),&bufoem, 0);

    Output("Welcome, this is HaRET running on WindowsCE v%ld.%ld\n"
           "Minimal virtual address: %p, maximal virtual address: %p",
           vi.dwMajorVersion, vi.dwMinorVersion,
           si.lpMinimumApplicationAddress, si.lpMaximumApplicationAddress);
    Output("Detected machine '%s' (Plat='%ls' OEM='%ls')\n"
           "CPU is %s running in %s mode\n"
           "Enter 'HELP' for a short command summary.\n",
           Mach->name, bufplat, bufoem,
           cpu_id(), cpu_mode());

    if (t.Initialize())
        for (int line = 1; ; line++) {
            // Some kind of prompt
            char prompt[16];
            _snprintf(prompt, sizeof(prompt), "HaRET(%d)# ", line);

            if (!t.Readline(prompt))
                break;

            if (!scrInterpret((char *)t.GetStr (), line))
                break;
        }

    setOutputFn(NULL);
}

// Listen for a connection on given port and execute commands
static void
scrListen(int port)
{
    prepThread();

    int lsock = socket(AF_INET, SOCK_STREAM, 0);
    if (lsock < 0) {
        Complain(C_ERROR("Failed to create socket"));
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int sock;

    if (bind(lsock, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
        Complain(C_ERROR("Failed to bind socket"));
        goto finish;
    }

    Status(L"Waiting for connection ...");

    if (listen(lsock, 1) < 0) {
        Complain(C_ERROR("Error on listen"));
        goto finish;
    }

    ShowCursor(TRUE);
    HCURSOR OldCursor;
    OldCursor = GetCursor();
    SetCursor(LoadCursor(NULL, IDC_WAIT));

    int addrlen;
    addrlen = sizeof(addr);
    sock = accept(lsock, (struct sockaddr *)&addr, &addrlen);

    if (sock < 0) {
        Complain(C_ERROR("Connection failed"));
        goto finish;
    }

    if (OldCursor != (HCURSOR)-1) {
        SetCursor(OldCursor);
        OldCursor = (HCURSOR)-1;
    }

    Status(L"Connect from %hs:%d", inet_ntoa (addr.sin_addr),
           htons (addr.sin_port));
    Screen("Incoming connection from %s:%d", inet_ntoa (addr.sin_addr),
           htons (addr.sin_port));

    // Close the gate
    so_close(lsock);
    lsock = -1;

    mainnetloop(sock);

    so_close(sock);

    Screen("Connection from %s:%d terminated", inet_ntoa(addr.sin_addr),
           htons(addr.sin_port));

finish:
    if (lsock != -1)
        so_close(lsock);

    Status(L"");
}

void
startListen(int port)
{
    CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)scrListen,
                 (LPVOID)port, 0, NULL);
}

static void
cmd_listen(const char *cmd, const char *args)
{
    uint32 port;
    if (!get_expression(&args, &port))
        port = 9999;
    startListen(port);
}
REG_CMD(0, "LISTEN", cmd_listen,
        "LISTEN [<port>]\n"
        "  Open a socket and wait for a connection on <port> (default 9999)")
