/*
    Simple telnet server emulation
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _TERMINAL_H
#define _TERMINAL_H

/* This class emulates a very basic telnet daemon. It is used so that it
 * eases a little the task of interacting with the telnet client that is
 * usually used to connect to HaRET and also to provide line editing
 * capabilities.
 */
class haretTerminal
{
  // The accumulated string
  uchar *str;
  // The length of the string
  size_t str_len;
  // The maximal length (allocated) for the string
  size_t str_max;
  // A temporary buffer for input
  uchar buff [64];
  // How much of the buffer is filled
  size_t buff_fill;

private:
  // Get data from client and return the number of bytes read or -1 on error
  virtual int Read (uchar *indata, size_t max_len) = 0;
  // Write a string to client and return bytes written or -1 on error
  virtual int Write (const uchar *outdata, size_t len) = 0;

public:
  // Initialize the object
  haretTerminal ();
  // Free memory
  virtual ~haretTerminal ();
  // Initialize the terminal (echo off etc)
  bool Initialize ();
  // Get the accumulated line
  const uchar *GetStr () { return str; }
  // Displays a prompt and read a line from client (allows editing input).
  // Returns false on premature EOF or read error.
  bool Readline (const char *prompt);
};

#endif /* _TERMINAL_H */
