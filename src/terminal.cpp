/*
    Simple telnet server emulation
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <string.h> // strlen, memmove
#include <stdlib.h> // realloc, free

#include "xtypes.h"
#include "terminal.h"

// Some telnet protocol special characters
#define _IAC  255
#define IAC   "\xff"
#define _WILL 251
#define WILL  "\xfb"
#define _WONT 252
#define WONT  "\xfc"
#define _DO   253
#define DO    "\xfd"
#define _DONT 254
#define DONT  "\xfe"

#define _BRK  243
#define BRK   "\xf3"
#define _ERSC 247
#define ERSC  "\xf7"
#define _ERSL 248
#define ERSL  "\xf8"

#define ECHO  "\x1"
#define SGA   "\x3" // SUPPRESS-GO-AHEAD

haretTerminal::haretTerminal ()
{
  str = NULL;
  str_len = 0;
  str_max = 0;
  buff_fill = 0;
}

haretTerminal::~haretTerminal ()
{
  if (str)
    free (str);
}

bool haretTerminal::Initialize ()
{
  Write ((const uchar *)IAC WILL ECHO IAC WILL SGA IAC DO SGA, 9);
  return true;
}

bool haretTerminal::Readline (const char *prompt)
{
  Write ((uchar *)prompt, strlen (prompt));

  // Reset string length
  size_t old_str_len = str_len;
  str_len = 0;

#define NEED(l) \
  { while (str_len + l > str_max) str = (uchar *)realloc (str, str_max += 32); }

  // Current mode
  int mode = 0;

  // Now read characters from input string until we encounter a newline
  for (;;)
  {
    if (!buff_fill)
    {
      int x = Read (buff, sizeof (buff));
      if (x <= 0)
        return false;
      buff_fill = x;
    }

    for (size_t i = 0; i < buff_fill; i++)
    {
      //wprintf (L"mode %d [%02x %c]\n", mode, buff [i], buff [i]);
      if (mode == 1)
      {
        switch (buff [i])
        {
          case '[':
            mode = 2;
            continue;
        }
        mode = 0;
      }
      else if (mode == 2)
      {
        switch (buff [i])
        {
          case 'A': // up
            str_len = 0;
            Write ((uchar *)"\r\x1b[K", 4);
            Write ((uchar *)prompt, strlen (prompt));
            break;
          case 'B': // down
            if (str_len < old_str_len)
            {
              Write (str + str_len, old_str_len - str_len);
              str_len = old_str_len;
            }
            break;
          case 'C': // right
            if (str_len < old_str_len)
            {
              Write (str + str_len, 1);
              str_len++;
            }
            break;
          case 'D': // left
            if (str_len > 0)
            {
              str_len--;
              Write ((uchar *)"\x1b[D \x1b[D", 7);
            }
            break;
        }
        mode = 0;
      }
      else if (mode == 255)
      {
        switch (buff [i])
        {
          case _ERSC: // Telnet Erase Character
            if (str_len > 0)
            {
              str_len--;
              Write ((uchar *)"\x1b[D \x1b[D", 7);
            }
            break;
          case _ERSL: // Telnet Erase Line
            str_len = 0;
            Write ((uchar *)"\r\x1b[K", 4);
            Write ((uchar *)prompt, strlen (prompt));
            break;
          case _DO:
          case _DONT:
          case _WILL:
          case _WONT:
            mode = 254;
            continue;
        }
        mode = 0;
      }
      else if (mode == 254)
        mode = 0;
      else
      {
        switch (buff [i])
        {
          case 0:
          case '\n':
            /* ignored */
            break;
          case '\r':
            /* Ok, set up the remains of the buffer and return ready */
            NEED (str_len + 1);
            str [str_len] = 0;
            memmove (buff, buff + i + 1, buff_fill - i - 1);
            buff_fill -= i + 1;
            Write ((uchar *)"\r\n", 2);
            return true;
          case 8:
          case 127:
            if (str_len > 0)
            {
              str_len--; old_str_len--;
              memmove (str + str_len, str + str_len + 1, old_str_len - str_len);
              Write ((uchar *)"\x1b[D \x1b[D", 7);
            }
            break;
          case 27:
            mode = 1;
            break;
          case _IAC:
            mode = 255;
            break;
          default:
            if (buff [i] >= ' ')
            {
              NEED (old_str_len + 1);
              memmove (str + str_len + 1, str + str_len, old_str_len - str_len);
              old_str_len++;
              str [str_len++] = buff [i];
              if (str_len > old_str_len)
                old_str_len = str_len;
              Write ((uchar *)buff + i, 1);
            }
            break;
        }
      }
    }
    buff_fill = 0;
  }
}
