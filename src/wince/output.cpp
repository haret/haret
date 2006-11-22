/*
    Poor Man's ARM Hardware Reverse Engineering Tool
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>
#include <windowsx.h> // Edit_SetSel
#include <commctrl.h> // TBM_SETRANGEMAX
#include <stdio.h> // vsnprintf
#include <ctype.h> // toupper

#include "resource.h" // ID_PROGRESSBAR
#include "script.h" // REG_CMD
#include "haret.h" // hInst, MainWindow
#include "output.h"

//#define USE_WAIT_CURSOR

// This function can be assigned a value in order to redirect
// message boxes elsewhere (such as to a socket)
void (*output_fn)(const char *msg) = NULL;

static void
Log(const char *msg)
{
  if (MainWindow == 0)
    return;

  wchar_t buff [512];
  _snwprintf(buff, sizeof(buff) / sizeof(buff[0]), L"%hs", msg);

  wchar_t *eol = buff + wcslen(buff);
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

  if (output_fn) {
    char buf[1024];
    _snprintf(buf, sizeof(buf), "%ls: %ls", title, buffer);
    output_fn(buf);
  } else
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

static FILE *outputLogfile = NULL;

void
__output(int sendScreen, const char *format, ...)
{
    char buf[512];

    va_list args;
    va_start(args, format);
    vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);

    if (outputLogfile) {
        fwprintf(outputLogfile, L"%hs\r\n", buf);
        fflush(outputLogfile);
    }
    if (sendScreen)
        Log(buf);
    if (output_fn)
        output_fn(buf);
}

static void
cmd_print(const char *tok, const char *x)
{
    bool msg = (toupper(tok[0]) == 'M');
    char *arg = _strdup(get_token(&x));
    uint32 args [4];
    for (int i = 0; i < 4; i++)
        if (!get_expression (&x, &args [i]))
            break;

    if (msg) {
        wchar_t tmp[200];
        _snwprintf(tmp, sizeof(tmp), C_INFO ("%hs"), arg);
        Complain(tmp, args [0], args [1], args [2], args [3]);
    } else {
        char tmp[200];
        _snprintf(tmp, sizeof(tmp), "%s", arg);
        Screen(tmp, args [0], args [1], args [2], args [3]);
    }
    free(arg);
}
REG_CMD(0, "M|ESSAGE", cmd_print,
        "MESSAGE <strformat> [<numarg1> [<numarg2> ... [<numarg4>]]]\n"
        "  Display a message (if run from a script, displays a message box).\n"
        "  <strformat> is a standard C format string (like in printf).\n"
        "  Note that to type a string you will have to use '%%hs'.")
REG_CMD_ALT(0, "P|RINT", cmd_print, print,
        "PRINT <strformat> [<numarg1> [<numarg2> ... [<numarg4>]]]\n"
        "  Same as MESSAGE except that it outputs the text without decorations\n"
        "  directly to the network pipe.")

// Request output to be copied to a local log file.
static int
openLogFile(const char *vn)
{
    char fn[200];
    fnprepare(vn, fn, sizeof(fn));
    if (outputLogfile)
        fclose(outputLogfile);
    outputLogfile = fopen(fn, "a");
    if (!outputLogfile)
        return -1;
    return 0;
}

static void
cmd_log(const char *cmd, const char *args)
{
    char *vn = get_token(&args);
    if (!vn) {
        Complain(C_ERROR("line %d: file name expected"), ScriptLine);
        return;
    }
    int ret = openLogFile(vn);
    if (ret)
        Output("line %d: Cannot open file `%s' for writing", ScriptLine, vn);
}
REG_CMD(0, "L|OG", cmd_log,
        "LOG <filename>\n"
        "  Log all output to specified file.")

// Close a previously opened log file.
void
closeLogFile()
{
    if (outputLogfile)
        fclose(outputLogfile);
    outputLogfile = NULL;
}

static void
cmd_unlog(const char *cmd, const char *args)
{
    closeLogFile();
}
REG_CMD(0, "UNL|OG", cmd_unlog,
        "UNLOG\n"
        "  Stop logging output to file.")

static char SourcePath[200];

static void
preparePath()
{
    // Locate the directory containing the haret executable.
    wchar_t sp[200];
    GetModuleFileName(hInst, sp, ARRAY_SIZE(sp));
    int len = wcstombs(SourcePath, sp, sizeof(SourcePath));
    char *x = SourcePath + len;
    while ((x > SourcePath) && (x[-1] != L'\\'))
        x--;
    *x = 0;
}

void
fnprepare(const char *ifn, char *ofn, int ofn_max)
{
    // Don't translate absolute file names
    if (ifn[0] == '\\')
        strncpy(ofn, ifn, ofn_max);
    else
        _snprintf(ofn, ofn_max, "%s%s", SourcePath, ifn);
}

// Initialize the output settings.
void
setupOutput()
{
    preparePath();

    char fn[100];
    fnprepare("earlyharetlog.txt", fn, sizeof(fn));
    FILE *logfd=fopen(fn, "r");
    if (logfd) {
        // Requesting early logs..
        fclose(logfd);
        openLogFile("haretlog.txt");
    }
    Output("Finished initializing output");
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

static uint LastProgress;

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

  LastProgress = 0;
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

  LastProgress = Value;
  SendMessage (slider, TBM_SETSELEND, TRUE, Value);
  return true;
}

bool AddProgress(int add)
{
    return SetProgress(LastProgress + add);
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
