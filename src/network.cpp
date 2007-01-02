/*
    Poor Man's Hardware Reverse Engineering Tool
    Network connection handling
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <stdio.h> // _snprintf

#include "xtypes.h"
#include "cpu.h" // printWelcome
#include "output.h" // Output, setOuptutFn
#include "terminal.h" // haretNetworkTerminal
#include "script.h" // scrInterpret
#include "machines.h" // Mach
#include "network.h"

#  include <winsock.h>
#  define so_close	closesocket

// Our private haretTerminal extension that reads/writes to socket
class haretNetworkTerminal : public haretTerminal, public outputfn
{
  int socket;

private:
  virtual int Read (uchar *indata, size_t max_len);
  virtual int Write (const uchar *outdata, size_t len);
  void sendMessage(const char *msg, int len);

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
haretNetworkTerminal::sendMessage(const char *msg, int len)
{
    send(socket, msg, len, 0);
}

static void
mainnetloop(int sock)
{
    haretNetworkTerminal t(sock);
    setOutputFn(&t);

    printWelcome();

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
        Output(C_ERROR "Failed to create socket");
        return;
    }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr("0.0.0.0");

    int sock;

    if (bind(lsock, (struct sockaddr *)&addr, sizeof (addr)) < 0) {
        Output(C_ERROR "Failed to bind socket");
        goto finish;
    }

    Status(L"Waiting for connection ...");

    if (listen(lsock, 1) < 0) {
        Output(C_ERROR "Error on listen");
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
        Output(C_ERROR "Connection failed");
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
