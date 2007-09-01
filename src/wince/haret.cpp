/*
    Poor Man's ARM Hardware Reverse Engineering Tool
    Copyright (C) 2003 Andrew Zabolotny
    Copyright (C) 2006 Kevin O'Connor <kevin@koconnor.net>
    Copyright (C) 2007 Paul Sokolovsky <pmiscml@gmail.com>

    For conditions of use see file COPYING
*/

/* The original intent of this program was to better understand how hardware
 * is configured/used in my Dell Axim handheld in order to make Linux boot
 * on it. But it was written generally enough to be used on other similar
 * hardware, so if you're going to port Linux to Yet Another PPC PDA, this
 * tool could give you some initial information :)
 */

#include <windows.h>
#include "pwindbas.h" // SetSystemMemoryDivision
#include <stdio.h> // _snwprintf
#include <wctype.h> // iswspace

#include "xtypes.h"
#include "resource.h" // DLG_HaRET
#include "output.h" // Output, setupOutput
#include "memory.h" // memPhysReset
#include "script.h" // scrExecute, setupCommands
#include "machines.h" // setupMachineType
#include "network.h" // scrListen
#include "linboot.h"

#define WINDOW_CLASS TEXT("pmret")
#define WINDOW_TITLE TEXT("HaRET")

static bool try_linboot(void);

HINSTANCE hInst;
HWND MainWindow = 0;
struct linload_header {
    char signature[8];
    int kernelSize;
    int initrdSize;
    int scriptSize;
    int kernelMd5;
    int initrdMd5;
    int scriptMd5;
};

static FILE *exeFile;
static struct linload_header header;

static BOOL CALLBACK DialogFunc (HWND hWnd, UINT message, WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
    {
      Output("In initdialog");
      MainWindow = hWnd;

      wchar_t title[200];
      _snwprintf(title, ARRAY_SIZE(title), L"HaRET Version %hs", VERSION);
      SetWindowText (hWnd, title);
      SetWindowText (GetDlgItem (hWnd, ID_SCRIPTNAME), L"default.txt");
      CheckDlgButton(hWnd, IDC_COM1, BST_CHECKED);
      ShowWindow (hWnd, SW_SHOWMAXIMIZED);
      Screen("Found machine %s", Mach->name);
      Output("executing startup.txt");
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
          wchar_t wscrfn[100];
	  GetWindowText(GetDlgItem (hWnd, ID_SCRIPTNAME), wscrfn,
                        ARRAY_SIZE(wscrfn));
          char scrfn[100];
          wcstombs(scrfn, wscrfn, sizeof(scrfn));
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
          startListen(9999);
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

    // Setup haret.
    setupHaret();

    if (try_linboot())
	return 0;

    // Initialize sockets
    Output("Running WSAStartup");
    WSADATA wsadata;
    WSAStartup(MAKEWORD(1, 1), &wsadata);

    /* To avoid fiddling with message queues et al we just fire up a
     * regular dialog window */
    Output("Starting gui");
    DialogBox(hInstance, MAKEINTRESOURCE(DLG_HaRET), HWND_DESKTOP, DialogFunc);

    Output("Calling WSACleanup");
    WSACleanup();

    shutdownHaret();

    return 0;
}


//----------

// Boot kernel linked into exe.
static void
ramboot(const char *cmd, const char *args)
{
    fseek(exeFile,
	  -(header.kernelSize + header.initrdSize + header.scriptSize + sizeof(struct linload_header)), 
	  SEEK_END);
    
    bootHandleLinux(exeFile, header.kernelSize, header.initrdSize);
}
REG_CMD(0, "RAMBOOT|LINUX", ramboot,
        "RAMBOOTLINUX\n"
        "  Start booting linux kernel. See HELP VARS for variables affecting boot.")

static char script_data[4096];

static void fatal(const char *msg)
{
    Output(msg);
    shutdownHaret();
    exit(1);
}	

static bool try_linboot(void)
{
    wchar_t exePathW[MAX_PATH];
    char exePath[MAX_PATH];
    
    // Read trailer
    GetModuleFileName(NULL, exePathW, MAX_PATH);
    wcstombs(exePath, exePathW, sizeof(exePath));
    exeFile = fopen(exePath, "rb");
    if (!exeFile)
	fatal(C_ERROR "Could not open executable");
    fseek(exeFile, -sizeof(struct linload_header), SEEK_END);
    int len = fread(&header, sizeof(struct linload_header), 1, exeFile);
    if (len != 1)
	fatal(C_ERROR "Could not read trailer");
    if (strncmp(header.signature, "HARET", 5))
	//fatal(C_ERROR "Could not find trailer signature");
	return false;
	
    STORE_INFORMATION storeInfo;
    if (GetStoreInformation(&storeInfo) && storeInfo.dwFreeSize > 100 * 1024) {
	/*DWORD ret = */SetSystemMemoryDivision((storeInfo.dwStoreSize - storeInfo.dwFreeSize) / 4096 + 50);
	/*if (ret == SYSMEM_CHANGED)
	    MessageBox(NULL, L"SYSMEM_CHANGED", L"SetSystemMemoryDivision", 1);
	else if (ret == SYSMEM_MUSTREBOOT)
	    MessageBox(NULL, L"SYSMEM_MUSTREBOOT", L"SetSystemMemoryDivision", 1);
	else
	    MessageBox(NULL, L"SYSMEM_FAILED", L"SetSystemMemoryDivision", 1);*/
    }

    fseek(exeFile,
	  -(header.scriptSize + sizeof(struct linload_header)), 
	  SEEK_END);
	
    int toRead = header.scriptSize < (int)sizeof(script_data) - 1 ? header.scriptSize : sizeof(script_data) - 1;
    len = fread(script_data, 1, toRead, exeFile);
    if (len != toRead)
	fatal(C_ERROR "Could not read script");

    // Run linked in script.
    runMemScript(script_data);

    // Shutdown.
    shutdownHaret();

    return true;
}
