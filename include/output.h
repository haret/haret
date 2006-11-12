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

// Initialize output logging code.
void setupOutput();
// Internal function for outputing to screen/logs/socket.
void __output(int sendScreen, const char *fmt, ...)
    __attribute__ ((format (printf, 2, 3)));
// Print some text through output_fn if set and/or log if set
#define Output(fmt, args...) __output(0, fmt , ##args )
// Send output to screen, output_fn (if set), and/or log (if set)
#define Screen(fmt, args...) __output(1, fmt , ##args )

// Create a named log file to send all Output strings to
int openLogFile(const char *vn);
// Close any previously created log files.
void closeLogFile();

extern bool InitProgress (uint Max);
extern bool SetProgress (uint Value);
extern void DoneProgress ();

// This function can be assigned a value in order to redirect
// message boxes elsewhere (such as to a socket)
extern void (*output_fn) (const char *msg);

#endif /* _MSGBOX_H */
