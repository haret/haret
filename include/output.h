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
#define C_LOG    "<9>"
#define C_NORM   "<7>"
#define C_SCREEN "<6>"
#define C_INFO   "<5>"
#define C_WARN   "<3>"
#define C_ERROR  "<0>"

/* Display some text in status line */
extern void Status (const wchar_t *format, ...);

// Prepend executable source directory to file name if it does not
// already contain a path.
extern void fnprepare(const char *ifn, char *ofn, int ofn_max);
// Version variable (added by make system)
extern const char *VERSION;
// Initialize Haret application.
void setupHaret();
void shutdownHaret();
void prepThread();
void Output(const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
// Send output to screen, output_fn (if set), and/or log (if set)
#define Screen(fmt, args...) Output(C_SCREEN fmt , ##args )

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
