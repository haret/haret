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
#include <winsock.h>

#include "xtypes.h"
#include "resource.h"
#include "output.h"
#include "memory.h"
#include "script.h"
#include "util.h"

#define WINDOW_CLASS TEXT("pmret")
#define WINDOW_TITLE TEXT("HaRET")

HINSTANCE hInst;
HWND MainWindow;
wchar_t SourcePath [200];

static BOOL CALLBACK DialogFunc (HWND hWnd, UINT message, WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      MainWindow = hWnd;

      wchar_t title [30];
      _snwprintf (title, sizeof (title) / sizeof (wchar_t),
                  L"HaRET Version %hs", VERSION);
      SetWindowText (hWnd, title);
      SetWindowText (GetDlgItem (hWnd, ID_SCRIPTNAME), L"default.txt");

      ShowWindow (hWnd, SW_SHOWMAXIMIZED);

      scrExecute ("startup.txt", false);
      return TRUE;
    }

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
          WideCharToMultiByte (CP_ACP, 0, wscrfn, wstrlen (wscrfn) + 1,
	    scrfn, sizeof (scrfn), " ", &flag);
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

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
  LPTSTR lpCmdLine, int nCmdShow)
{
  hInst = hInstance;

  GetModuleFileName (hInst, SourcePath, sizeof (SourcePath) / sizeof (wchar_t));
  wchar_t *x = wstrchr (SourcePath, 0);
  while ((x > SourcePath) && (x [-1] != L'\\'))
    x--;
  *x = 0;

  // Initialize sockets
  WSADATA wsadata;
  WSAStartup (MAKEWORD(1, 1), &wsadata);

  /* To avoid fiddling with message queues et al we just fire up a
     regular dialog window */
  DialogBox (hInstance, MAKEINTRESOURCE (DLG_HaRET), HWND_DESKTOP,
    DialogFunc);

  memPhysReset ();
  WSACleanup ();

  return 0;
}
