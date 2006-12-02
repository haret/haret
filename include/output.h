/*
    Error display
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _MSGBOX_H
#define _MSGBOX_H

#include "xtypes.h" // uint

#define C_INFO(s)	L"<6>" L##s
#define C_WARN(s)	L"<3>" L##s
#define C_ERROR(s)	L"<0>" L##s

/* Complain about something. You can prepend one of the C_XXX macros
  (see below) to format to denote different severity levels. */
extern void Complain (const wchar_t *format, ...);
/* Display some text in status line */
extern void Status (const wchar_t *format, ...);

// Prepend executable source directory to file name if it does not
// already contain a path.
extern void fnprepare(const char *ifn, char *ofn, int ofn_max);
// Initialize output logging code.
void setupOutput();
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

extern bool InitProgress (uint Max);
extern bool SetProgress (uint Value);
extern bool AddProgress(int add);
extern void DoneProgress ();

// Class used to direct output to listeners.
class outputfn {
public:
    virtual ~outputfn() {}
    virtual void sendMessage(const char *msg) = 0;
};

// Setup the output function for this thread.
outputfn *setOutputFn(outputfn *ofn);

#endif /* _MSGBOX_H */
