/*
    Poor Man's ARM Hardware Reverse Engineering Tool
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>

#include "output.h"
#include "util.h"
#include "resource.h"

extern HINSTANCE hInst;
extern HWND MainWindow;

// This function can be assigned a value in order to redirect
// message boxes elsewhere (such as to a socket)
void (*output_fn) (wchar_t *msg, wchar_t *title) = NULL;

//#define USE_WAIT_CURSOR

void Log (const wchar_t *format, ...)
{
  wchar_t buff [512];
  va_list args;
  va_start (args, format);
  _vsnwprintf (buff, sizeof (buff) / sizeof (wchar_t), format, args);
  va_end (args);

  wchar_t *eol = wstrchr (buff, 0);
  // Append a newline at the end
  *eol++ = L'\n'; *eol = 0;
  wchar_t *dst = buff + sizeof (buff) / sizeof (wchar_t);

  // Convert \n to \r\n
  while (eol >= buff)
  {
    if (dst <= eol)
      break;

    dst--;
    if (*eol == L'\n')
    {
      *dst-- = L'\n';
      *dst = L'\r';
    }
    else
      *dst = *eol;
    eol--;
  }

  HWND hConsole = GetDlgItem (MainWindow, ID_LOG);
  uint maxlen = SendMessage (hConsole, EM_GETLIMITTEXT, 0, 0);

  uint tl, sl = buff + (sizeof (buff) / sizeof (wchar_t)) - dst;

  while ((tl = GetWindowTextLength (hConsole)) + sl >= maxlen)
  {
    uint linelen = SendMessage (hConsole, EM_LINELENGTH, 0, 0) + 2;
    Edit_SetSel (hConsole, 0, linelen);
    Edit_ReplaceSel (hConsole, "");
  }

  Edit_SetSel (hConsole, tl, tl);
  Edit_ReplaceSel (hConsole, dst);
}

/* Handy printf-like functions for displaying messages.
 * Message severity can be specified by concatenating a
 * C_XXX macro before the format string (same as KERN_WARN
 * and so on).
 */
void Complain (const wchar_t *format, ...)
{
  unsigned severity = MB_ICONEXCLAMATION;
  wchar_t *title = L"Warning";

  if (format [0] == L'<'
   && format [1] >= L'0'
   && format [1] <= L'9'
   && format [2] == L'>')
  {
    if (format [1] >= L'6')
      severity = MB_ICONASTERISK, title = L"Information";
    else if (format [1] >= L'3')
      /* default value */;
    else
      severity = MB_ICONHAND, title = L"Error";
    format += 3;
  }

  wchar_t buffer [512];
  va_list args;
  va_start (args, format);
  _vsnwprintf (buffer, sizeof (buffer) / sizeof (wchar_t), format, args);
  va_end (args);

  if (output_fn)
    output_fn (buffer, title);
  else
    MessageBox (0, buffer, title, MB_OK | MB_APPLMODAL | severity);
}

void Status (const wchar_t *format, ...)
{
  wchar_t buffer [512];
  va_list args;
  va_start (args, format);
  _vsnwprintf (buffer, sizeof (buffer) / sizeof (wchar_t), format, args);
  va_end (args);

  HWND sb = GetDlgItem (MainWindow, ID_STATUSTEXT);
  if (sb)
    SetWindowText (sb, buffer);
}

void Output (const wchar_t *format, ...)
{
  wchar_t buffer [512];
  va_list args;
  va_start (args, format);
  _vsnwprintf (buffer, sizeof (buffer) / sizeof (wchar_t), format, args);
  va_end (args);

  if (output_fn)
    output_fn (buffer, NULL);
}

static HWND pb;
#ifdef USE_WAIT_CURSOR
static HCURSOR OldCursor;
#endif

static BOOL CALLBACK pbDialogFunc (HWND hWnd, UINT message, WPARAM wParam,
  LPARAM lParam)
{
  switch (message)
  {
    case WM_INITDIALOG:
      return TRUE;

    case WM_COMMAND:
      switch (LOWORD (wParam))
      {
        case IDOK:
        case IDCANCEL:
          EndDialog (hWnd, LOWORD (wParam));
          return TRUE;
      }
      break;
 }
  return FALSE;
}

bool InitProgress (uint Max)
{
#ifdef USE_WAIT_CURSOR
  OldCursor = (HCURSOR)-1;
#endif

  pb = CreateDialog (hInst, MAKEINTRESOURCE (DLG_PROGRESS), MainWindow,
                     pbDialogFunc);
  if (!pb)
    return false;

  HWND slider = GetDlgItem (pb, ID_PROGRESSBAR);
  if (!slider)
  {
    DoneProgress ();
    pb = NULL;
    return false;
  }

#ifdef USE_WAIT_CURSOR
  ShowCursor (TRUE);
  OldCursor = GetCursor ();
  SetCursor (LoadCursor (NULL, IDC_WAIT));
#endif

  SendMessage (slider, TBM_SETRANGEMAX, TRUE, Max);
  SendMessage (slider, TBM_SETTICFREQ, 10, 0);
  return true;
}

bool SetProgress (uint Value)
{
  if (!pb)
    return false;

  HWND slider = GetDlgItem (pb, ID_PROGRESSBAR);
  if (!slider)
    return false;

  SendMessage (slider, TBM_SETSELEND, TRUE, Value);
  return true;
}

void DoneProgress ()
{
  if (pb)
  {
    DestroyWindow (pb);
    pb = NULL;
  }

#ifdef USE_WAIT_CURSOR
  if (OldCursor != (HCURSOR)-1)
  {
    SetCursor (OldCursor);
    OldCursor = (HCURSOR)-1;
  }
#endif
}
