/*
    Poor Man's ARM Hardware Reverse Engineering Tool
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

/* The original intent of this program was to better understand how hardware
 * is configured/used in my Dell Axim handheld in order to make Linux boot
 * on it. But it was written generally enough to be used on other similar
 * hardware, so if you're going to port Linux to Yet Another PPC PDA, this
 * tool could give you some initial information :)
 */

#include <windows.h>
#include <stdio.h> // _snwprintf
#include <wctype.h> // iswspace

#include "xtypes.h"
#include "resource.h" // DLG_HaRET
#include "output.h" // Output, setupOutput
#include "memory.h" // memPhysReset
#include "script.h" // scrExecute, setupCommands
#include "machines.h" // setupMachineType
#include "network.h" // scrListen

#define WINDOW_CLASS TEXT("pmret")
#define WINDOW_TITLE TEXT("HaRET")

HINSTANCE hInst;
HWND MainWindow = 0;

static BOOL CALLBACK DialogFunc (HWND hWnd, UINT message, WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      Output("In initdialog");
      MainWindow = hWnd;

      wchar_t title [30];
      _snwprintf (title, sizeof (title) / sizeof (wchar_t),
                  L"HaRET Version %hs", VERSION);
      SetWindowText (hWnd, title);
      SetWindowText (GetDlgItem (hWnd, ID_SCRIPTNAME), L"default.txt");
      CheckDlgButton(hWnd, IDC_COM1, BST_CHECKED);
      ShowWindow (hWnd, SW_SHOWMAXIMIZED);
      Screen("Found machine %s", Mach->name);
      Output("executing startup.txt");
      scrExecute ("startup.txt", false);
      return TRUE;
    }
    case WM_CANCELMODE:
          EndDialog (hWnd, LOWORD (wParam));
          return TRUE;
    case WM_COMMAND:
      switch (LOWORD (wParam))
      {
        case IDOK:
        case IDCANCEL:
          EndDialog (hWnd, LOWORD (wParam));
          return TRUE;
        case BT_SCRIPT:
        {
          wchar_t wscrfn [100];
	  GetWindowText (GetDlgItem (hWnd, ID_SCRIPTNAME), wscrfn,
	    sizeof (wscrfn) / sizeof (wchar_t));
          char scrfn [100];
          BOOL flag;
          WideCharToMultiByte (CP_ACP, 0, wscrfn, -1,
	    scrfn, sizeof (scrfn), " ", &flag);
          MoveWindow (hWnd, 0, 0, GetSystemMetrics (SM_CXSCREEN),
            GetSystemMetrics (SM_CYSCREEN), FALSE);
          ShowWindow (hWnd, SW_SHOW);
          SetForegroundWindow (hWnd);
          scrExecute (scrfn);
          return TRUE;
	}
        case BT_LISTEN:
        {
          // Call scrListen (9999) in a new thread
          CreateThread (NULL, 0, (LPTHREAD_START_ROUTINE)scrListen,
            (LPVOID)9999, 0, NULL);
          return TRUE;
        }
      }
      break;
  }

  return FALSE;
}

int WINAPI
WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
         LPTSTR lpCmdLine, int nCmdShow)
{
    hInst = hInstance;

    // Prep for early output.
    setupOutput();

    // Detect some system settings
    setupMachineType();

    // Setup variable/command lists.
    setupCommands();

    // Initialize sockets
    Output("Running WSAStartup");
    WSADATA wsadata;
    WSAStartup(MAKEWORD(1, 1), &wsadata);

    /* To avoid fiddling with message queues et al we just fire up a
     * regular dialog window */
    Output("Starting gui");
    DialogBox(hInstance, MAKEINTRESOURCE(DLG_HaRET), HWND_DESKTOP, DialogFunc);

    Output("Shutting down");
    memPhysReset();
    WSACleanup();
    closeLogFile();

    return 0;
}
