/*
    Poor Man's ARM Hardware Reverse Engineering Tool
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h>
#include "pkfuncs.h" // SetKMode
#include <windowsx.h> // Edit_SetSel
#include <commctrl.h> // TBM_SETRANGEMAX
#include <stdio.h> // vsnprintf
#include <ctype.h> // toupper

#include "machines.h" // setupMachineType
#include "memory.h" // mem_autodetect
#include "lateload.h" // setup_LateLoading
#include "resource.h" // ID_PROGRESSBAR
#include "script.h" // REG_CMD, setupCommands
#include "haret.h" // hInst, MainWindow
#include "output.h"

//#define USE_WAIT_CURSOR

static const int MAXOUTBUF = 2*1024;
static const int PADOUTBUF = 32;


/****************************************************************
 * Functions for sending messages to screen.
 ****************************************************************/

static void
writeScreen(const char *msg, int len)
{
  if (MainWindow == 0)
    return;

  wchar_t buff[MAXOUTBUF];
  mbstowcs(buff, msg, ARRAY_SIZE(buff));

  HWND hConsole = GetDlgItem (MainWindow, ID_LOG);
  uint maxlen = SendMessage (hConsole, EM_GETLIMITTEXT, 0, 0);

  uint tl;

  while ((tl = GetWindowTextLength (hConsole)) + len >= maxlen)
  {
    uint linelen = SendMessage (hConsole, EM_LINELENGTH, 0, 0) + 2;
    Edit_SetSel (hConsole, 0, linelen);
    Edit_ReplaceSel (hConsole, "");
  }

  Edit_SetSel (hConsole, tl, tl);
  Edit_ReplaceSel (hConsole, buff);
}

void Status (const wchar_t *format, ...)
{
  wchar_t buffer [512];
  va_list args;
  va_start (args, format);
  _vsnwprintf (buffer, ARRAY_SIZE(buffer), format, args);
  va_end (args);

  HWND sb = GetDlgItem (MainWindow, ID_STATUSTEXT);
  if (sb)
    SetWindowText (sb, buffer);
}

static void
Complain(const char *msg, int len, int code)
{
    unsigned severity = MB_ICONEXCLAMATION;
    wchar_t *title = L"Warning";

    if (code >= 6) {
        severity = MB_ICONASTERISK;
        title = L"Information";
    } else if (code >= 3) {
        /* default value */
    } else {
        severity = MB_ICONHAND;
        title = L"Error";
    }

    wchar_t buffer[512];
    mbstowcs(buffer, msg, ARRAY_SIZE(buffer));

    MessageBox(0, buffer, title, MB_OK | MB_APPLMODAL | severity);
}


/****************************************************************
 * Log file operations
 ****************************************************************/

static HANDLE outputLogfile;

static void
writeLog(const char *msg, uint32 len)
{
    if (!outputLogfile)
        return;
    DWORD nw;
    WriteFile(outputLogfile, msg, len, &nw, 0);
}

// Request output to be copied to a local log file.
static int
openLogFile(const char *vn)
{
    char fn[200];
    fnprepare(vn, fn, sizeof(fn));
    if (outputLogfile)
        CloseHandle(outputLogfile);
    wchar_t wfn[200];
    mbstowcs(wfn, fn, ARRAY_SIZE(wfn));
    outputLogfile = CreateFile(wfn, GENERIC_WRITE, FILE_SHARE_READ
                               , 0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if (!outputLogfile)
        return -1;
    return 0;
}

// Ask wince to write log fully to disk (like an fsync).
void
flushLogFile()
{
    if (outputLogfile)
        FlushFileBuffers(outputLogfile);
}

// Close a previously opened log file.
void
closeLogFile()
{
    if (outputLogfile)
        CloseHandle(outputLogfile);
    outputLogfile = NULL;
}


/****************************************************************
 * Main Output() code
 ****************************************************************/

static DWORD outTls;

static inline outputfn *getOutputFn(void) {
    return (outputfn*)TlsGetValue(outTls);
}

outputfn *
setOutputFn(outputfn *ofn)
{
    outputfn *old = getOutputFn();
    TlsSetValue(outTls, (void*)ofn);
    return old;
}

static int
convertNL(char *outbuf, int maxlen, const char *inbuf, int len)
{
    // Convert CR to CR/LF since telnet requires this
    const char *s = inbuf, *s_end = &inbuf[len];
    char *d = outbuf;
    char *d_end = &outbuf[maxlen - 3];
    while (s < s_end && d < d_end) {
        if (*s == '\n')
            *d++ = '\r';
        *d++ = *s++;
    }

    // A trailing tab character is an indicator to not add in a
    // trailing newline - in all other cases add the newline.
    if (d > outbuf && d[-1] == '\t') {
        d--;
    } else {
        *d++ = '\r';
        *d++ = '\n';
    }

    *d = '\0';
    return d - outbuf;
}

void
__output(int sendScreen, const char *format, ...)
{
    // Check for error indicator (eg, format starting with "<0>")
    int code = 0;
    if (format[0] == '<'
        && format[1] >= '0' && format[1] <= '9'
        && format[2] == '>') {
        code = format[1] - '0' + 1;
        format += 3;
    }

    // Format output string.
    char rawbuf[MAXOUTBUF];
    va_list args;
    va_start(args, format);
    int rawlen = vsnprintf(rawbuf, sizeof(rawbuf) - PADOUTBUF, format, args);
    va_end(args);

    // Convert newline characters
    char buf[MAXOUTBUF];
    int len = convertNL(buf, sizeof(buf), rawbuf, rawlen);

    writeLog(buf, len);
    if (sendScreen)
        writeScreen(buf, len);
    outputfn *ofn = getOutputFn();
    if (ofn) {
        ofn->sendMessage(buf, len);
    } else if (code) {
        Complain(rawbuf, rawlen, code-1);
    }
}


/****************************************************************
 * Path setup and output init
 ****************************************************************/

static char SourcePath[200];

void
fnprepare(const char *ifn, char *ofn, int ofn_max)
{
    // Don't translate absolute file names
    if (ifn[0] == '\\')
        strncpy(ofn, ifn, ofn_max);
    else
        _snprintf(ofn, ofn_max, "%s%s", SourcePath, ifn);
}

static void
preparePath()
{
    // Locate the directory containing the haret executable.
    wchar_t sp[200];
    GetModuleFileName(hInst, sp, ARRAY_SIZE(sp));
    int len = wcstombs(SourcePath, sp, sizeof(SourcePath));
    char *x = SourcePath + len;
    while ((x > SourcePath) && (x[-1] != '\\'))
        x--;
    *x = 0;
}

// Prepare thread for general availability.
void
prepThread()
{
    // Set per-thread output function to NULL (for CE 2.1 machines
    // where this isn't the default.)
    TlsSetValue(outTls, 0);

    // All wince 3.0 and later machines are automatically in "kernel
    // mode".  We enable kernel mode by default to make older PDAs
    // (ce2.x) work.
    Output("Setting KMode to true.");
    int kmode = SetKMode(TRUE);
    Output("Old KMode was %d", kmode);
}

// Initialize the haret application.
void
setupHaret()
{
    preparePath();

    // Open log file "haretlog.txt" if "earlyharetlog.txt" is found.
    char fn[100];
    fnprepare("earlyharetlog.txt", fn, sizeof(fn));
    FILE *logfd=fopen(fn, "r");
    if (logfd) {
        // Requesting early logs..
        fclose(logfd);
        openLogFile("haretlog.txt");
    }

    // Prep for per-thread output function.
    outTls = TlsAlloc();
    prepThread();

    Output("Finished initializing output");

    // Bind to DLLs dynamically.
    setup_LateLoading();

    // Detect the memory on the machine via wince first.
    Output("Detecting memory");
    mem_autodetect();

    // Detect some system settings
    setupMachineType();

    // Setup variable/command lists.
    setupCommands();
}


/****************************************************************
 * Progress bar functions
 ****************************************************************/

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


/****************************************************************
 * Misc commands.
 ****************************************************************/

static void
cmd_print(const char *tok, const char *x)
{
    bool msg = (toupper(tok[0]) == 'M');
    char *arg = _strdup(get_token(&x));
    uint32 args[4];
    for (int i = 0; i < 4; i++)
        if (!get_expression(&x, &args[i]))
            break;

    const char *fmt = arg;
    char tmp[200];
    if (msg) {
        _snprintf(tmp, sizeof(tmp), C_INFO "%s", arg);
        fmt = tmp;
    }

    __output(!msg, fmt, args[0], args[1], args[2], args[3]);

    free(arg);
}
REG_CMD(0, "M|ESSAGE", cmd_print,
        "MESSAGE <strformat> [<numarg1> [<numarg2> ... [<numarg4>]]]\n"
        "  Display a message (if run from a script, displays a message box).\n"
        "  <strformat> is a standard C format string (like in printf).")
REG_CMD_ALT(0, "P|RINT", cmd_print, print,
        "PRINT <strformat> [<numarg1> [<numarg2> ... [<numarg4>]]]\n"
        "  Same as MESSAGE except that it outputs the text without decorations\n"
        "  directly to the network pipe.")

static void
cmd_log(const char *cmd, const char *args)
{
    char *vn = get_token(&args);
    if (!vn) {
        Output(C_ERROR "line %d: file name expected", ScriptLine);
        return;
    }
    int ret = openLogFile(vn);
    if (ret)
        Output("line %d: Cannot open file `%s' for writing", ScriptLine, vn);
}
REG_CMD(0, "L|OG", cmd_log,
        "LOG <filename>\n"
        "  Log all output to specified file.")

static void
cmd_unlog(const char *cmd, const char *args)
{
    closeLogFile();
}
REG_CMD(0, "UNL|OG", cmd_unlog,
        "UNLOG\n"
        "  Stop logging output to file.")
