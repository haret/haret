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
#include <Tlhelp32.h>

#include "xtypes.h"
#include "resource.h"
#include "output.h"
#include "memory.h"
#include "script.h"
#include "util.h"
#include "cpu.h"
#include "com_port.h"

#define VERSION "0.3.7"
#define WINDOW_CLASS TEXT("pmret")
#define WINDOW_TITLE TEXT("HaRET")

HINSTANCE hInst;
HWND MainWindow = 0;
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
      CheckDlgButton(hWnd, IDC_COM1, BST_CHECKED);
      ShowWindow (hWnd, SW_SHOWMAXIMIZED);
      cpuDetect ();
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
          WideCharToMultiByte (CP_ACP, 0, wscrfn, wstrlen (wscrfn) + 1,
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

/* avoid useless LCD calibration 950 times per session :) */
void kill_welcome ()
{
  HANDLE hts = INVALID_HANDLE_VALUE;
  HANDLE hproc = INVALID_HANDLE_VALUE;
  PROCESSENTRY32 pe;
  pe.dwSize = sizeof (PROCESSENTRY32);

  hts = CreateToolhelp32Snapshot (TH32CS_SNAPPROCESS, 0);

  if (Process32First (hts, &pe))
  {
    do
    {
      if (wcsicmp (L"welcome.exe", pe.szExeFile) == 0)
      {
        hproc = OpenProcess (0, 0, pe.th32ProcessID);

        if (hproc != INVALID_HANDLE_VALUE && hproc != NULL)
          break;
      }
    } while (Process32Next (hts, &pe));
  }

  if (hproc != INVALID_HANDLE_VALUE && hproc != NULL )
  {
    TerminateProcess (hproc, 0);
    CloseHandle (hproc);
  }

  CloseToolhelp32Snapshot (hts);
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

  /* commandline parsing */
  wchar_t *p;
  int run = 0;
  int kill = 0;

  p = lpCmdLine;

  while (*p)
  {
    while(*p && iswspace(*p))
      p++;

    if(!*p)
      break;

    if (p[0] == '-' && p[1] == 'r')
      run = 1;
    if (p[0] == '-' && p[1] == 'k')
      kill = 1;

    p++;
  }

  /* kill LCD calibration app */
  if (kill == 1)
    kill_welcome();

  if (run != 1)
  {
    /* To avoid fiddling with message queues et al we just fire up a regular dialog window */
    DialogBox (hInstance, MAKEINTRESOURCE (DLG_HaRET), HWND_DESKTOP, DialogFunc);
  }
  else
  {
    /* allow immediate linux boot */
    cpuDetect ();
    scrExecute ("startup.txt", false);
    scrExecute ("default.txt");
  }

  memPhysReset ();
  WSACleanup ();

  return 0;
}
