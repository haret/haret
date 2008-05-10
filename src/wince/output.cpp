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
#include "cpu.h" // printWelcome
#include "exceptions.h" // init_ehandling
#include "output.h"

//#define USE_WAIT_CURSOR

static const int MAXOUTBUF = 2*1024;
static const int PADOUTBUF = 32;


/****************************************************************
 * Functions for sending messages to screen.
 ****************************************************************/

static void
writeScreen(const char *msg, uint len)
{
    if (MainWindow == 0)
        return;

    wchar_t buff[MAXOUTBUF];
    mbstowcs(buff, msg, ARRAY_SIZE(buff));

    HWND hConsole = GetDlgItem(MainWindow, ID_LOG);
    uint maxlen = SendMessage(hConsole, EM_GETLIMITTEXT, 0, 0);

    if (len >= maxlen)
        // Message wont fit on screen.
        return;

    // Remove lines until there is space in screen log
    uint tl;
    for (;;) {
        tl = GetWindowTextLength(hConsole);
        if (tl + len < maxlen)
            break;
        uint linelen = SendMessage(hConsole, EM_LINELENGTH, 0, 0) + 2;
        Edit_SetSel(hConsole, 0, linelen);
        Edit_ReplaceSel(hConsole, "");
    }

    // Paste message to screen log
    Edit_SetSel(hConsole, tl, tl);
    Edit_ReplaceSel(hConsole, buff);
}

void
Status(const wchar_t *format, ...)
{
    wchar_t buffer[512];
    va_list args;
    va_start(args, format);
    _vsnwprintf(buffer, ARRAY_SIZE(buffer), format, args);
    va_end(args);

    HWND sb = GetDlgItem(MainWindow, ID_STATUSTEXT);
    if (sb)
        SetWindowText(sb, buffer);
}

static void
Complain(const char *msg, int len, int code)
{
    unsigned severity = MB_ICONEXCLAMATION;
    wchar_t *title = L"Warning";

    if (code >= 5) {
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
    outputLogfile = CreateFile(wfn, GENERIC_WRITE, FILE_SHARE_READ,
			       0, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL /*| FILE_FLAG_WRITE_THROUGH*/, 0);
    if (!outputLogfile)
        return -1;
    // Append to log
    SetFilePointer(outputLogfile, 0, NULL, FILE_END);
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
static void
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

// Output message to screen/logs/socket.
void
Output(const char *format, ...)
{
    // Check for error indicator (eg, format starting with "<0>")
    int code = 7;
    if (format[0] == '<'
        && format[1] >= '0' && format[1] <= '9'
        && format[2] == '>') {
        code = format[1] - '0';
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
    outputfn *ofn = getOutputFn();
    if (!ofn && code < 6) {
        Complain(rawbuf, rawlen, code-1);
        return;
    }
    if (code <= 6)
        writeScreen(buf, len);
    if (ofn && code <= 7)
        ofn->sendMessage(buf, len);
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

    init_thread_ehandling();

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

    outTls = TlsAlloc();
    TlsSetValue(outTls, 0);

    Output("\n===== HaRET %s =====", VERSION);

    init_ehandling();

    // Prep for per-thread output function.
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

    // Initialize the machine found earlier.
    Output("Initializing for machine '%s'", Mach->name);
    Mach->init();

    // Send banner info to log (if logging on).
    printWelcome();
}

// Final steps to run on haret shutdown.
void
shutdownHaret()
{
    Output("Shutting down");
    memPhysReset();
    closeLogFile();
}


/****************************************************************
 * Progress bar functions
 ****************************************************************/

// Maximum number of updates to the progress bar.
enum { PB_MAXSTEPS = 20 };

static struct ProgressFeedback {
    HWND window, slider;
    HCURSOR oldCursor;
    uint lastProgress, lastShownProgress, showStep;
} progressFeedback;

static BOOL CALLBACK
pbDialogFunc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    switch (message) {
    case WM_INITDIALOG:
        return TRUE;

    case WM_COMMAND:
        switch (LOWORD(wParam)) {
        case IDOK:
        case IDCANCEL:
            EndDialog(hWnd, LOWORD(wParam));
            return TRUE;
        }
        break;
    }
    return FALSE;
}

bool InitProgress(int dialogId, uint Max)
{
#ifdef USE_WAIT_CURSOR
    progressFeedback.oldCursor = (HCURSOR)-1;
#endif

    progressFeedback.window = CreateDialog(hInst, MAKEINTRESOURCE(dialogId)
                                           , MainWindow, pbDialogFunc);
    if (!progressFeedback.window)
        return false;

    progressFeedback.slider = GetDlgItem(progressFeedback.window, dialogId + 1);
    if (!progressFeedback.slider) {
        DoneProgress();
        return false;
    }

#ifdef USE_WAIT_CURSOR
    ShowCursor(TRUE);
    progressFeedback.oldCursor = GetCursor();
    SetCursor(LoadCursor(NULL, IDC_WAIT));
#endif

    progressFeedback.lastProgress = 0;
    progressFeedback.lastShownProgress = 0;
    progressFeedback.showStep = Max / PB_MAXSTEPS;
    SendMessage(progressFeedback.slider, TBM_SETRANGEMAX, TRUE, PB_MAXSTEPS);
    return true;
}

bool SetProgress(uint Value)
{
    if (!progressFeedback.slider)
        return false;

    progressFeedback.lastProgress = Value;
    uint stepval = Value / progressFeedback.showStep;
    if (stepval != progressFeedback.lastShownProgress) {
        progressFeedback.lastShownProgress = stepval;
        SendMessage(progressFeedback.slider, TBM_SETSELEND, TRUE, stepval);
    }
    return true;
}

bool AddProgress(int add)
{
    return SetProgress(progressFeedback.lastProgress + add);
}

void DoneProgress()
{
    if (progressFeedback.window) {
        DestroyWindow(progressFeedback.window);
        progressFeedback.window = NULL;
        progressFeedback.slider = NULL;
    }

#ifdef USE_WAIT_CURSOR
    if (progressFeedback.oldCursor != (HCURSOR)-1) {
        SetCursor(progressFeedback.oldCursor);
        progressFeedback.oldCursor = (HCURSOR)-1;
    }
#endif
}


/****************************************************************
 * Misc commands.
 ****************************************************************/

static void
cmd_print(const char *tok, const char *args)
{
    char buf[MAX_CMDLEN];
    int ret = arg_snprintf(buf, sizeof(buf), args);
    if (ret)
        return;

    if (toupper(tok[0]) == 'M')
        Output(C_INFO "%s", buf);
    else
        Output("%s", buf);
}
REG_CMD(0, "M|ESSAGE", cmd_print,
        "MESSAGE <strformat> [<numarg1> [<numarg2> ... [<numarg4>]]]\n"
        "  Display a message (if run from a script, displays a message box).\n"
        "  <strformat> is a standard C format string (like in printf).")
REG_CMD_ALT(0, "P|RINT", cmd_print, print,
        "PRINT <strformat> [<numarg1> [<numarg2> ... [<numarg4>]]]\n"
        "  Same as MESSAGE except that it outputs the text without decorations\n"
        "  directly to the network pipe.")

static uint32 MsgBoxResult;
REG_VAR_INT(0, "RESULT", MsgBoxResult
            , "Return code from MSGBOX command")

static void
cmd_msgbox(const char *tok, const char *x)
{
    wchar_t title[MAX_CMDLEN];
    wchar_t msg[MAX_CMDLEN];
    uint32 flags = 0;

    get_wtoken(&x, title, ARRAY_SIZE(title));
    get_wtoken(&x, msg, ARRAY_SIZE(msg));
    get_expression(&x, &flags);

    MsgBoxResult = MessageBox(0, msg, title, flags);
}
REG_CMD(0, "MSGBOX", cmd_msgbox,
        "MSGBOX <title> <msg> <flags>\n"
        "  Display a message box. Button and icon appearance specified by flags.\n"
        "  Store result code to var RESULT.")

static void
cmd_log(const char *cmd, const char *args)
{
    char vn[MAX_CMDLEN];
    if (get_token(&args, vn, sizeof(vn))) {
        ScriptError("file name expected");
        return;
    }
    int ret = openLogFile(vn);
    if (ret)
        ScriptError("Cannot open file `%s' for writing", vn);
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
