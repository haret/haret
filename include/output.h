/*
    Error display
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _MSGBOX_H
#define _MSGBOX_H

#include "xtypes.h" // uint

// Prepend one of the following strings to the format of an Output
// call to force a messagebox to popup.
#define C_INFO  "<6>"
#define C_WARN  "<3>"
#define C_ERROR "<0>"

/* Display some text in status line */
extern void Status (const wchar_t *format, ...);

// Prepend executable source directory to file name if it does not
// already contain a path.
extern void fnprepare(const char *ifn, char *ofn, int ofn_max);
// Version variable (added by make system)
extern const char *VERSION;
// Initialize Haret application.
void setupHaret();
void prepThread();
// Internal function for outputing to screen/logs/socket.
void __output(int sendScreen, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
// Print some text through output_fn if set and/or log if set
#define Output(fmt, args...) __output(0, fmt , ##args )
// Send output to screen, output_fn (if set), and/or log (if set)
#define Screen(fmt, args...) __output(1, fmt , ##args )

// Close any previously created log files.
void closeLogFile();
// Flush log file writes to disk.
void flushLogFile();

extern bool InitProgress(int dialogId, uint Max);
extern bool SetProgress(uint Value);
extern bool AddProgress(int add);
extern void DoneProgress();

// Class used to direct output to listeners.
class outputfn {
public:
    virtual ~outputfn() {}
    virtual void sendMessage(const char *msg, int len) = 0;
};

// Setup the output function for this thread.
outputfn *setOutputFn(outputfn *ofn);

#endif /* _MSGBOX_H */
