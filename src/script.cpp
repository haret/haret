/*
    Poor Man's Hardware Reverse Engineering Tool script language interpreter
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "xtypes.h"
#include "script.h"
#include "memory.h"
#include "video.h"
#include "output.h"
#include "util.h"
#include "cpu.h"
#include "gpio.h"
#include "linboot.h"

static const char *quotes = "\"'";
// Currently processed line (for error display)
static uint line;

#define ScriptVarsCount (sizeof (ScriptVars) / sizeof (varDescriptor))
static varDescriptor ScriptVars [] =
{
  { "MMU", "Memory Management Unit level 1 descriptor table physical addr",
    varROFunc, (uint32 *)&cpuGetMMU, 0 },
  { "PID", "Current Process Identifier register value",
    varROFunc, (uint32 *)&cpuGetPID, 0 },
  { "VRAM", "Video Memory physical address",
    varROFunc, (uint32 *)&vidGetVRAM, 0 },
  { "PSR", "Program Status Register",
    varROFunc, (uint32 *)&cpuGetPSR, 0 },
  { "KERNEL", "Linux kernel file name",
    varString, (uint32 *)&bootKernel },
  { "INITRD", "Initial Ram Disk file name",
    varString, (uint32 *)&bootInitrd },
  { "CMDLINE", "Kernel command line",
    varString, (uint32 *)&bootCmdline },
  { "IGPIO", "The list of GPIOs to ignore during WATCHGPIO",
    varBitSet, (uint32 *)&gpioIgnore, 84 },
  { "BOOTSPD", "Boot animation speed, usec/scanline (0-no delay)",
    varInteger, &bootSpeed },
  { "MTYPE", "ARM machine type (see linux/arch/arm/tools/mach-types)",
    varInteger, &bootMachineType },
  { "RAMADDR", "Physical RAM start address (default = 0xa0000000)",
    varInteger, &memPhysAddr },
  { "RAMSIZE", "Physical RAM size (default = autodetected)",
    varInteger, &memPhysSize },
  { "V2P", "Virtual To Physical address translation",
    varROFunc, (uint32 *)&memVirtToPhys, 1 },
  { "P2V", "Physical To Virtual address translation",
    varROFunc, (uint32 *)&memPhysMap, 1 },
  { "CP", "Coprocessor Registers access",
    varROFunc, (uint32 *)&cpuScrCP, 2 },
  { "VMB", "Virtual Memory Byte access",
    varRWFunc, (uint32 *)&memScrVMB, 1 },
  { "VMH", "Virtual Memory Halfword access",
    varRWFunc, (uint32 *)&memScrVMH, 1 },
  { "VMW", "Virtual Memory Word access",
    varRWFunc, (uint32 *)&memScrVMW, 1 },
  { "PMB", "Physical Memory Byte access",
    varRWFunc, (uint32 *)&memScrPMB, 1 },
  { "PMH", "Physical Memory Halfword access",
    varRWFunc, (uint32 *)&memScrPMH, 1 },
  { "PMW", "Physical Memory Word access",
    varRWFunc, (uint32 *)&memScrPMW, 1 },
  { "GPSR", "General Purpose I/O State Register",
    varRWFunc, (uint32 *)&gpioScrGPSR, 1 },
  { "GPDR", "General Purpose I/O Direction Register",
    varRWFunc, (uint32 *)&gpioScrGPDR, 1 },
  { "GAFR", "General Purpose I/O Alternate Function Select Register",
    varRWFunc, (uint32 *)&gpioScrGAFR, 1 }
};

#define ScriptDumpersCount (sizeof (ScriptDumpers) / sizeof (hwDumper))
static struct hwDumper ScriptDumpers [] =
{
  { "GPIO", "GPIO machinery state in a human-readable format.",
    0, gpioDump },
  { "GPIOST", "GPIO state suitable for include/asm/arch/xxx-init.h",
    0, gpioDumpState },
  { "CP", "Value of 16 coprocessor registers (arg = coproc number)",
    1, cpuDumpCP },
  { "MMU", "Virtual memory map (4Gb address space).",
    0, memDumpMMU }
};

static varDescriptor *UserVars = NULL;
static int UserVarsCount = 0;

static char *get_token (const char **s)
{
  const char *x = *s;
  static char storage [100];

  // Skip spaces at the beginning
  while (*x && isspace (*x))
    x++;

  // If at the end of string, return empty token
  if (!*x)
  {
    storage [0] = 0;
    return storage;
  }

  char quote;
  bool has_quote = !!strchr (quotes, quote = *x);
  if (has_quote)
    x++;

  const char *e = x;
  if (has_quote)
    while (*e && (*e != quote))
      e++;
  else
    while (*e && isalnum (*e))
      e++;

  if (e >= x + sizeof (storage))
    e = x + sizeof (storage) - 1;
  memcpy (storage, x, e - x);
  storage [e - x] = 0;

  if (has_quote && *e)
    e++;
  *s = e;

  return storage;
}

static char peek_char (const char **s)
{
  const char *x = *s;

  // Skip spaces at the beginning
  while (*x && isspace (*x))
    x++;

  *s = x;
  return *x;
}

static void BitSet (uint32 *bs, uint num, bool state)
{
  uint32 ofs = num >> 5;
  uint32 mask = 1 << (num & 31);
  if (state)
    bs [ofs] |= mask;
  else
    bs [ofs] &= ~mask;
}

static bool BitGet (uint32 *bs, uint num)
{
  uint32 ofs = num >> 5;
  uint32 mask = 1 << (num & 31);
  return !!(bs [ofs] & mask);
}

static bool get_args (const char **s, const char *keyw, uint32 *args,
                      uint count);

static varDescriptor *FindVar (const char *vn, varDescriptor *Vars, int VarCount)
{
  for (int i = 0; i < VarCount; i++)
    if (!_stricmp (vn, Vars [i].name))
      return Vars + i;
  return NULL;
}

static bool GetVar (const char *vn, const char **s, uint32 *v,
                    varDescriptor *Vars, int VarCount)
{
  varDescriptor *var = FindVar (vn, Vars, VarCount);
  if (!var)
    return false;

  switch (var->type)
  {
    case varInteger:
      *v = *var->ival;
      break;
    case varString:
      *v = (uint32)*var->sval;
      break;
    case varBitSet:
      if (!get_args (s, vn, v, 1))
        return false;
      if (*v > var->val_size)
      {
        Complain (C_ERROR ("line %d: Index out of range (0..%d)"),
                  line, var->val_size);
        return false;
      }
      *v = BitGet (var->bsval, *v);
      break;
    case varROFunc:
    case varRWFunc:
    {
      uint32 args [50];
      if (var->val_size)
      {
        if (!get_args (s, vn, args, var->val_size))
          return false;
      }
      *v = var->fval (false, args, 0);
      break;
    }
  }
  return true;
}

varDescriptor *NewVar (char *vn, varType vt)
{
  varDescriptor *ouv = UserVars;
  UserVars = (varDescriptor *)
      realloc (UserVars, sizeof (varDescriptor) * (UserVarsCount + 1));
  if (UserVars != ouv)
  {
    // Since we reallocated the stuff, we have to fix the ival pointers as well
    for (int i = 0; i < UserVarsCount; i++)
      if (UserVars [i].type == varInteger)
        UserVars [i].ival = &UserVars [i].val_size;
  }

  varDescriptor *nv = UserVars + UserVarsCount;
  memset (nv, 0, sizeof (*nv));
  nv->name = strnew (vn);
  nv->type = vt;
  // Since integer variables don't use their val_size field,
  // we'll use it for variable value itself
  nv->ival = &nv->val_size;
  UserVarsCount++;
  return nv;
}

// Quick primitive expression evaluator
// Operation priorities:
// 0: ()
// 1: + - | ^
// 2: * / % &
// 3: unary+ unary- ~

// Expect to see a ')'
#define PAREN_EXPECT	1
// Eat the closing ')'
#define PAREN_EAT	2

static bool get_expression (const char **s, uint32 *v, int priority = 0, int flags = 0)
{
  uint32 b;
  char *x = get_token (s);

  if (!*x)
  {
    // Got empty token, could be a unary operator or a parenthesis
    switch (peek_char (s))
    {
      case '(':
        (*s)++;
        if (!get_expression (s, v, 0, PAREN_EAT | PAREN_EXPECT))
          return false;
        break;

      case '+':
        (*s)++;
        if (!get_expression (s, v, 3, flags & ~PAREN_EAT))
          return false;
        break;

      case '-':
        (*s)++;
        if (!get_expression (s, v, 3, flags & ~PAREN_EAT))
          return false;
        *v = (uint32)-(int32)*v;
        break;

      case '~':
        (*s)++;
        if (!get_expression (s, v, 3, flags & ~PAREN_EAT))
          return false;
        *v = ~*v;
        break;

      case 0:
      case ',':
        return false;

      default:
        Complain (C_ERROR ("line %d: Unexpected input '%hs'"), line, *s);
        return false;
    }
  }
  else
  {
    if (*x >= '0' && *x <= '9')
    {
      // We got a number
      char *err;
      *v = strtoul (x, &err, 0);
      if (*err)
      {
        Complain (C_ERROR ("line %d: Expected a number, got %hs"), line, x);
        return false;
      }
    }
    // Look through variables
    else if (!GetVar (x, s, v, ScriptVars, ScriptVarsCount)
          && !GetVar (x, s, v, UserVars, UserVarsCount))
    {
      Complain (C_ERROR ("line %d: Unknown variable '%hs' in expression"),
                line, x);
      return false;
    }
  }

  // Peek next char and see if it is a operator
  bool unk_op = false;
  while (!unk_op)
  {
    char op = peek_char (s);
    switch (op)
    {
      case '+':
      case '-':
      case '|':
      case '^':
        if (priority > 1)
          return true;
        (*s)++;
        if (!get_expression (s, &b, 1, flags & ~PAREN_EAT))
          return false;
        switch (op)
        {
          case '+': *v += b; break;
          case '-': *v -= b; break;
          case '|': *v |= b; break;
          case '^': *v ^= b; break;
        }
        break;

      case '*':
      case '/':
      case '%':
      case '&':
        if (priority > 2)
          return true;
        (*s)++;
        if (!get_expression (s, &b, 2, flags & ~PAREN_EAT))
          return false;
        switch (op)
        {
          case '*': *v *= b; break;
          case '/': *v /= b; break;
          case '%': *v %= b; break;
          case '&': *v &= b; break;
        }
        break;

      case ')':
        if (!(flags & PAREN_EXPECT))
        {
          Complain (C_ERROR ("line %d: Unexpected ')'"), line);
          return false;
        }
        if (flags & PAREN_EAT)
          (*s)++;
        return true;

      default:
        unk_op = true;
        break;
    }
  }

  if (flags & PAREN_EXPECT)
  {
    Complain (C_ERROR ("line %d: No closing ')'"), line);
    return false;
  }

  return true;
}

static bool get_args (const char **s, const char *keyw, uint32 *args, uint count)
{
  if (!count)
    return true;

  if (peek_char (s) != '(')
  {
    Complain (C_ERROR ("line %d: %s(%d args) expected"), line, keyw, count);
    return false;
  }

  // keyw gets destroyed in next call to get_expression
  char *kw = strnew (keyw);

  (*s)++;
  while (count--)
  {
    if (!get_expression (s, args, 0, count ? 0 : PAREN_EXPECT | PAREN_EAT))
    {
error:
      Complain (C_ERROR ("line %d: not enough arguments to function %hs"), line, kw);
      delete [] kw;
      return false;
    }

    if (!count)
      break;

    if (peek_char (s) != ',')
      goto error;

    (*s)++;
    args++;
  }

  delete [] kw;
  return true;
}

// Compare the token to a mask that separates the mandatory part from
// optional suffix with a '|', e.g. 'VD|UMP'
static bool IsToken (const char *tok, const char *mask)
{
  while (*tok && *mask && toupper (*tok) == *mask)
    tok++, mask++;

  if (!*tok && !*mask)
    return true;

  if (*mask != '|')
    return false;

  mask++;
  while (*tok && *mask && toupper (*tok) == *mask)
    tok++, mask++;

  if (*tok)
    return false;

  return true;
}

/* fprintf-like function for dumpers */
static void conwrite (void *data, const char *format, ...)
{
  char buff [512];

  va_list args;
  va_start (args, format);
  _vsnprintf (buff, sizeof (buff), format, args);
  va_end (args);

  Output (L"%hs\t", buff);
}

bool scrInterpret (const char *str, uint lineno)
{
  line = lineno;

  const char *x = str;
  while (*x && isspace (*x))
    x++;
  if (*x == '#' || !*x)
    return true;

  char *tok = get_token (&x);

  // Okay, now see what keyword is this :)
  if (IsToken (tok, "M|ESSAGE")
   || IsToken (tok, "P|RINT"))
  {
    bool msg = (toupper (tok [0]) == 'M');
    char *arg = strnew (get_token (&x));
    uint32 args [4];
    for (int i = 0; i < 4; i++)
      if (!get_expression (&x, &args [i]))
        break;

    wchar_t tmp [200];
    _snwprintf (tmp, sizeof (tmp), msg ? C_INFO ("%hs") : L"%hs", arg);
    if (msg)
      Complain (tmp, args [0], args [1], args [2], args [3]);
    else
      Output (tmp, args [0], args [1], args [2], args [3]);
    Log (tmp, args [0], args [1], args [2], args [3]);
    delete [] arg;
  }
  else if (IsToken (tok, "VDU|MP")
        || IsToken (tok, "PDU|MP")
        || IsToken (tok, "VD")
        || IsToken (tok, "PD"))
  {
    bool virt = toupper (tok [0]) == 'V';
    char *fn = NULL;
    if (tok [2])
     fn = get_token (&x);

    uint32 addr, size;
    if (!get_expression (&x, &addr)
     || !get_expression (&x, &size))
    {
      Complain (C_ERROR ("line %d: Expected %hs<vaddr> <size>"),
                line, fn ? "<fname>" : "");
      return true;
    }

    if (virt)
      memDump (fn, (uint8 *)addr, size);
    else
      memPhysDump (fn, addr, size);
  }
  else if (IsToken (tok, "VFB")
        || IsToken (tok, "VFH")
        || IsToken (tok, "VFW")
        || IsToken (tok, "PFB")
        || IsToken (tok, "PFH")
        || IsToken (tok, "PFW"))
  {
    uint32 addr, size, value;

    char fill_type = toupper (tok [0]);
    char fill_size = toupper (tok [2]);

    if (!get_expression (&x, &addr)
     || !get_expression (&x, &size)
     || !get_expression (&x, &value))
    {
      Complain (C_ERROR ("line %d: Expected <addr> <size> <value>"), line);
      return true;
    }

    switch (fill_size)
    {
      case 'B':
        value &= 0xff;
        value = value | (value << 8) | (value << 16) | (value << 24);
        size = (size + 3) >> 2;
        break;

      case 'H':
        value &= 0xffff;
        value = value | (value << 16);
        size = (size + 1) >> 1;
        break;
    }

    switch (fill_type)
    {
      case 'V':
        memFill ((uint32 *)addr, size, value);
        break;

      case 'P':
        memPhysFill (addr, size, value);
        break;
    }
  }
  else if (IsToken (tok, "PWF")
        || IsToken (tok, "VWF"))
  {
    bool virt = toupper (tok [0]) == 'V';
    char *fn = strnew (get_token (&x));
    if (!fn)
    {
      Complain (C_ERROR ("line %d: file name expected"), line);
      return true;
    }

    uint32 addr, size;
    if (!get_expression (&x, &addr)
     || !get_expression (&x, &size))
    {
      delete [] fn;
      Complain (C_ERROR ("line %d: Expected <address> <size>"), line);
      return true;
    }

    if (virt)
      memVirtWriteFile (fn, addr, size);
    else
      memPhysWriteFile (fn, addr, size);
    delete [] fn;
  }
  else if (IsToken (tok, "D|UMP"))
  {
    char *vn = get_token (&x);
    if (!vn || !*vn)
    {
      Output (L"line %d: Dumper name expected", line);
      return true;
    }

    hwDumper *hwd = NULL;
    for (int i = 0; i < ScriptDumpersCount; i++)
      if (!_stricmp (vn, ScriptDumpers [i].name))
      {
        hwd = ScriptDumpers + i;
        break;
      }
    if (!hwd)
    {
      Output (L"line %d: No dumper %hs available, see HELP DUMP for a list", line, vn);
      return true;
    }

    uint32 args [50];
    if (!get_args (&x, hwd->name, args, hwd->nargs))
      return true;

    vn = get_token (&x);
    if (vn && !*vn)
      vn = NULL;

    FILE *f = NULL;
    if (vn)
    {
      char fn [200];
      fnprepare (vn, fn, sizeof (fn));

      f = fopen (fn, "wb");
      if (!f)
      {
        Output (L"line %d: Cannot open file `%hs' for writing", line, fn);
        return true;
      }
    }

    hwd->dump (f ? (void (*) (void *, const char *, ...))fprintf : conwrite,
               f, args);
    fclose (f);
  }
  else if (IsToken (tok, "W|ATCHGPIO"))
  {
    uint32 sec;
    if (!get_expression (&x, &sec))
    {
      Complain (C_ERROR ("line %d: Expected <seconds>"), line);
      return true;
    }
    gpioWatch (sec);
  }
  else if (IsToken (tok, "S|ET"))
  {
    char *vn = get_token (&x);
    if (!*vn)
    {
      Complain (C_ERROR ("line %d: Expected either <varname> or `LIST'"), line);
      return true;
    }

    varDescriptor *var = FindVar (vn, ScriptVars, ScriptVarsCount);
    if (!var)
      var = FindVar (vn, UserVars, UserVarsCount);
    if (!var)
      var = NewVar (vn, varInteger);

    switch (var->type)
    {
      case varInteger:
        if (!get_expression (&x, var->ival))
        {
          Complain (C_ERROR ("line %d: Expected numeric <value>"), line);
          return true;
        }
        break;
      case varString:
        // If val_size is zero, it means a const char* in .text segment
        if (var->val_size)
          delete [] var->sval;
        *var->sval = strnew (get_token (&x));
        var->val_size = 1;
        break;
      case varBitSet:
      {
        uint32 idx, val;
        if (!get_expression (&x, &idx)
         || !get_expression (&x, &val))
        {
          Complain (C_ERROR ("line %d: Expected <index> <value>"), line);
          return true;
        }
        if (idx > var->val_size)
        {
          Complain (C_ERROR ("line %d: Index out of range (0..%d)"),
                    line, var->val_size);
          return true;
        }
        BitSet (var->bsval, idx, !!val);
        break;
      }
      case varRWFunc:
      {
        uint32 val;
        uint32 args [50];

        if (!get_args (&x, var->name, args, var->val_size))
          return true;
        if (!get_expression (&x, &val))
        {
          Complain (C_ERROR ("line %d: Expected <value>"), line);
          return true;
        }
        var->fval (true, args, val);
        break;
      }
      default:
        Complain (C_ERROR ("line %d: `%hs' is a read-only variable"), line,
                 var->name);
        return true;
    }
  }
  else if (IsToken (tok, "BOOT|LINUX"))
  {
    bootLinux ();
  }
  else if (IsToken (tok, "H|ELP"))
  {
    char *vn = get_token (&x);

    if (!_stricmp (vn, "VARS"))
    {
      char type [9];
      char args [10];

      Output (L"Name\tType\tDescription");
      Output (L"-----------------------------------------------------------");
      for (size_t i = 0; i < ScriptVarsCount; i++)
      {
        args [0] = 0;
        type [0] = 0;
        switch (ScriptVars [i].type)
        {
          case varInteger:
            strcpy (type, "int");
            break;
          case varString:
            strcpy (type, "string");
            break;
          case varBitSet:
            strcpy (type, "bitset");
            break;
          case varROFunc:
            strcpy (type, "ro func");
	    // fallback
          case varRWFunc:
    	    if (!type [0])
	      strcpy (type, "rw func");
            if (ScriptVars [i].val_size)
	      sprintf (args, "(%d)", ScriptVars [i].val_size);
            break;
        }
        Output (L"%hs%hs\t%hs\t%hs", ScriptVars [i].name, args, type,
                ScriptVars [i].desc);
      }
    }
    else if (!_stricmp (vn, "DUMP"))
    {
      char args [10];
      for (size_t i = 0; i < ScriptDumpersCount; i++)
      {
        if (ScriptDumpers [i].nargs)
          sprintf (args, "(%d)", ScriptDumpers [i].nargs);
        else
          args [0] = 0;
        Output (L"%hs%hs\t%hs", ScriptDumpers [i].name, args,
                ScriptDumpers [i].desc);
      }
    }
    else if (!vn || !*vn)
    {
      Output (L"----=====****** A summary of HaRET commands: ******=====----");
      Output (L"Notations used below:");
      Output (L"  [A|B] denotes either A or B");
      Output (L"  <ABC> denotes a mandatory argument");
      Output (L"  Any command name can be shortened to minimal unambiguous length,");
      Output (L"  e.g. you can use 'p' for 'priint' but not 'vd' for 'vdump'");
      Output (L"BOOTLINUX");
      Output (L"  Start booting linux kernel. See HELP VARS for variables affecting boot.");
      Output (L"DUMP <hardware>[(args...)] [filename]");
      Output (L"  Dump the state of given hardware to given file (or to connection if");
      Output (L"  no filename specified). Use HELP DUMP to see available dumpers.");
      Output (L"HELP [VARS|DUMP]");
      Output (L"  Display a description of either commands, variables or dumpers.");
      Output (L"MESSAGE <strformat> [<numarg1> [<numarg2> ... [<numarg4>]]]");
      Output (L"  Display a message (if run from a script, displays a message box).");
      Output (L"  <strformat> is a standard C format string (like in printf).");
      Output (L"  Note that to type a string you will have to use '%%hs'.");
      Output (L"PRINT <strformat> [<numarg1> [<numarg2> ... [<numarg4>]]]");
      Output (L"  Same as MESSAGE except that it outputs the text without decorations");
      Output (L"  directly to the network pipe.");
      Output (L"QUIT");
      Output (L"  Quit the remote session.");
      Output (L"SET <variable> <value>");
      Output (L"  Assign a value to a variable. Use HELP VARS for a list of variables.");
      Output (L"[V|P]DUMP <filename> <addr> <size>");
      Output (L"  Dump an area of memory in hexadecimal/char format from given [V]irtual");
      Output (L"  or [P]hysical address to specified file.");
      Output (L"[V|P]D <addr> <size>");
      Output (L"  Same as [V|P]DUMP but outputs to screen rather than to file.");
      Output (L"[V|P]F[B|H|W] <addr> <count> <value>");
      Output (L"  Fill memory at given [V]irtual or [P]hysical address with a value.");
      Output (L"  The [B]yte/[H]alfword/[W]ord suffixes selects the size of");
      Output (L"  <value> and in which units the <count> is measured.");
      Output (L"[V|P]WF <filename> <addr> <size>");
      Output (L"  Write a portion of [V]irtual or [P]hysical memory to given file.");
      Output (L"WATCHGPIO <seconds>");
      Output (L"  Watch GPIO pins for given period of time and report changes.");
    }
    else
      Output (L"No help on this topic available");
  }
  else if (IsToken (tok, "Q|UIT"))
    return false;
  else
    Complain (C_ERROR ("Unknown keyword: `%hs'"), tok);

  return true;
}

void scrExecute (const char *scrfn, bool complain)
{
  char fn [100];
  fnprepare (scrfn, fn, sizeof (fn));

  FILE *f = fopen (fn, "r");
  if (!f)
  {
    if (complain)
      Complain (C_ERROR ("Cannot open script file\n%hs"), fn);
    return;
  }

  for (line = 1; ; line++)
  {
    char str [200];
    if (!fgets (str, sizeof (str), f))
      break;

    char *x = strchr (str, 0);
    while ((x [-1] == '\n') || (x [-1] == '\r'))
      *(--x) = 0;

    scrInterpret (str, line);
  }

  fclose (f);
}
