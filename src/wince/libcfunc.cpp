/* Some libc functions not present on all versions of wince.
 *
 * This file may be distributed under the terms of the GNU GPL license.
 */

#include <ctype.h> // defs, toupper
#include <limits.h> // ULONG_MAX
#include <string.h> // defs
#include <stdlib.h> // defs

// The functions in this file are not implmented in coredll.dll on
// some WinCE 2.x devices (despite what the msdn docs say).  As such,
// these local copies of the functions are used instead.


// _isctype adopted from Wine code - Copyright 2000 Jon Griffiths

#define _C_ _CONTROL
#define _S_ _SPACE
#define _P_ _PUNCT
#define _D_ _DIGIT
#define _H_ _HEX
#define _U_ _UPPER
#define _L_ _LOWER

static short haret__ctype[257] = {
  0, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|_C_, _S_|_C_,
  _S_|_C_, _S_|_C_, _S_|_C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_,
  _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _C_, _S_|_BLANK,
  _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _P_,
  _P_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_, _D_|_H_,
  _D_|_H_, _D_|_H_, _D_|_H_, _P_, _P_, _P_, _P_, _P_, _P_, _P_, _U_|_H_,
  _U_|_H_, _U_|_H_, _U_|_H_, _U_|_H_, _U_|_H_, _U_, _U_, _U_, _U_, _U_,
  _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_, _U_,
  _U_, _P_, _P_, _P_, _P_, _P_, _P_, _L_|_H_, _L_|_H_, _L_|_H_, _L_|_H_,
  _L_|_H_, _L_|_H_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_,
  _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _L_, _P_, _P_, _P_, _P_,
  _C_, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
  0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
};

int _isctype(int c, int type)
{
    return haret__ctype[c+1] & type;
}

int
_stricmp(const char *str1, const char *str2)
{
  int i;
   for (i=0; str1[i] && str2[i] && toupper(str1[i])==toupper(str2[i]); i++);
    if (str1[i]=='\0' && str2[i]=='\0')
     return 0;

   return -1;
}

/* modified dietlibc implementation */
unsigned long int
strtoul(const char *ptr, char **endptr, int base)
{
  int neg = 0, overflow = 0;
  unsigned long int v = 0;
  const char *orig;
  const char *nptr = ptr;

  while (isspace (*nptr))
    ++nptr;

  if (*nptr == '-')
    {
      neg = 1;
      nptr++;
    }
  else if (*nptr == '+')
    ++nptr;
  orig = nptr;
  if (base == 16 && nptr[0] == '0')
    goto skip0x;
  if (base)
    {
      register unsigned int b = base - 2;
      if (b > 34)
	{
	  return 0;
	}
    }
  else
    {
      if (*nptr == '0')
	{
	  base = 8;
	skip0x:
	  if ((nptr[1] == 'x' || nptr[1] == 'X') && isxdigit (nptr[2]))
	    {
	      nptr += 2;
	      base = 16;
	    }
	}
      else
	base = 10;
    }
  while (*nptr)
    {
      unsigned char c = *nptr;
      c =
	((c >= 'a') ? (c - 'a' + 10) : c >= 'A' ? c - 'A' + 10 : (c <='9') ? (c - '0') : 0xff);
      if (c >= base)
	break;			/* out of base */
      {
	unsigned long x = (v & 0xff) * base + c;
	unsigned long w = (v >> 8) * base + (x >> 8);
	if (w > (ULONG_MAX >> 8))
	  overflow = 1;
	v = (w << 8) + (x & 0xff);
      }
      ++nptr;
    }
  if (nptr == orig)
    {				/* no conversion done */
      nptr = ptr;
      ;
      v = 0;
    }
  if (endptr)
    *endptr = (char *) nptr;
  if (overflow)
    {
      ;
      return ULONG_MAX;
    }
//  return (neg ? -v : v);
  return v;
}

