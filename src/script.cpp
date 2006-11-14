/*
    Poor Man's Hardware Reverse Engineering Tool script language interpreter
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <stdio.h> // fopen, FILE
#include <ctype.h> // isspace, toupper
#include <stdarg.h> // va_list

#include "xtypes.h"
#include "script.h"
#include "output.h" // Output, Complain
#include "util.h" // fnprepare

static const char *quotes = "\"'";
// Currently processed line (for error display)
uint ScriptLine;

// Symbols added by linker.
extern "C" {
    extern varDescriptor vars_start[];
    extern varDescriptor vars_end;
    extern haret_cmd_s commands_start[];
    extern haret_cmd_s commands_end;
    extern hwDumper dumpcommands_start[];
    extern hwDumper dumpcommands_end;
}
#define vars_count (&vars_end - vars_start)
#define commands_count (&commands_end - commands_start)
#define dumpcommands_count (&dumpcommands_end - dumpcommands_start)

static varDescriptor *UserVars = NULL;
static int UserVarsCount = 0;

char *
get_token(const char **s)
{
  const char *x = *s;
  static char storage [1000];

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
    if (!strcasecmp (vn, Vars [i].name))
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
                  ScriptLine, var->val_size);
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

bool
get_expression(const char **s, uint32 *v, int priority, int flags)
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
        Complain (C_ERROR ("line %d: Unexpected input '%hs'"), ScriptLine, *s);
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
        Complain (C_ERROR ("line %d: Expected a number, got %hs"), ScriptLine, x);
        return false;
      }
    }
    // Look through variables
    else if (!GetVar (x, s, v, vars_start, vars_count)
          && !GetVar (x, s, v, UserVars, UserVarsCount))
    {
      Complain (C_ERROR ("line %d: Unknown variable '%hs' in expression"),
                ScriptLine, x);
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
          Complain (C_ERROR ("line %d: Unexpected ')'"), ScriptLine);
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
    Complain (C_ERROR ("line %d: No closing ')'"), ScriptLine);
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
    Complain (C_ERROR ("line %d: %hs(%d args) expected"), ScriptLine, keyw, count);
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
      Complain (C_ERROR ("line %d: not enough arguments to function %hs"), ScriptLine, kw);
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

  Output("%s\t", buff);
}

bool scrInterpret (const char *str, uint lineno)
{
    ScriptLine = lineno;

    const char *x = str;
    while (*x && isspace(*x))
        x++;
    if (*x == '#' || !*x)
        return true;

    char *tok = get_token(&x);

    // Okay, now see what keyword is this :)
    for (int i = 0; i < commands_count; i++)
        if (IsToken(tok, commands_start[i].name)) {
            commands_start[i].func(tok, x);
            return true;
        }

    if (IsToken(tok, "Q|UIT"))
        return false;
    else
        Complain(C_ERROR("Unknown keyword: `%hs'"), tok);

    return true;
}

static void
cmd_dump(const char *cmd, const char *x)
{
    char *vn = get_token (&x);
    if (!vn || !*vn)
    {
        Output("line %d: Dumper name expected", ScriptLine);
        return;
    }

    hwDumper *hwd = NULL;
    for (int i = 0; i < dumpcommands_count; i++)
        if (!strcasecmp (vn, dumpcommands_start[i].name))
        {
            hwd = dumpcommands_start + i;
            break;
        }
    if (!hwd)
    {
        Output("line %d: No dumper %s available, see HELP DUMP for a list", ScriptLine, vn);
        return;
    }

    uint32 args [50];
    if (!get_args (&x, hwd->name, args, hwd->nargs))
        return;

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
            Output("line %d: Cannot open file `%s' for writing", ScriptLine, fn);
            return;
        }
    }

    hwd->dump (f ? (void (*) (void *, const char *, ...))fprintf : conwrite,
               f, args);
    fclose (f);
}
REG_CMD(0, "D|UMP", cmd_dump,
        "DUMP <hardware>[(args...)] [filename]\n"
        "  Dump the state of given hardware to given file (or to connection if\n"
        "  no filename specified). Use HELP DUMP to see available dumpers.")

static void
cmd_set(const char *cmd, const char *x)
{
    char *vn = get_token (&x);
    if (!*vn)
    {
        Complain (C_ERROR ("line %d: Expected either <varname> or `LIST'"), ScriptLine);
        return;
    }

    varDescriptor *var = FindVar (vn, vars_start, vars_count);
    if (!var)
        var = FindVar (vn, UserVars, UserVarsCount);
    if (!var)
        var = NewVar (vn, varInteger);

    switch (var->type)
    {
    case varInteger:
        if (!get_expression (&x, var->ival))
        {
            Complain (C_ERROR ("line %d: Expected numeric <value>"), ScriptLine);
            return;
        }
        break;
    case varString:
        // If val_size is zero, it means a const char* in .text segment
        if (var->val_size)
            free(*var->sval);
        *var->sval = strnew (get_token (&x));
        var->val_size = 1;
        if (var->notify_set != NULL)
            (var->notify_set)();
        break;
    case varBitSet:
    {
        uint32 idx, val;
        if (!get_expression (&x, &idx)
            || !get_expression (&x, &val))
        {
            Complain (C_ERROR ("line %d: Expected <index> <value>"), ScriptLine);
            return;
        }
        if (idx > var->val_size)
        {
            Complain (C_ERROR ("line %d: Index out of range (0..%d)"),
                      ScriptLine, var->val_size);
            return;
        }
        BitSet (var->bsval, idx, !!val);
        break;
    }
    case varRWFunc:
    {
        uint32 val;
        uint32 args [50];

        if (!get_args (&x, var->name, args, var->val_size))
            return;
        if (!get_expression (&x, &val))
        {
            Complain (C_ERROR ("line %d: Expected <value>"), ScriptLine);
            return;
        }
        var->fval (true, args, val);
        break;
    }
    default:
        Complain (C_ERROR ("line %d: `%hs' is a read-only variable"), ScriptLine,
                  var->name);
        return;
    }
}
REG_CMD(0, "S|ET", cmd_set,
        "SET <variable> <value>\n"
        "  Assign a value to a variable. Use HELP VARS for a list of variables.")

static void
cmd_help(const char *cmd, const char *x)
{
    char *vn = get_token (&x);

    if (!strcasecmp (vn, "VARS"))
    {
        char type [9];
        char args [10];

        Output("Name\tType\tDescription");
        Output("-----------------------------------------------------------");
        for (int i = 0; i < vars_count; i++)
        {
            args [0] = 0;
            type [0] = 0;
            switch (vars_start [i].type)
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
                if (vars_start [i].val_size)
                    sprintf (args, "(%d)", vars_start [i].val_size);
                break;
            }
            Output("%s%s\t%s\t%s", vars_start [i].name, args, type,
                   vars_start [i].desc);
        }
    }
    else if (!strcasecmp (vn, "DUMP"))
    {
        char args [10];
        for (int i = 0; i < dumpcommands_count; i++)
        {
            if (dumpcommands_start[i].nargs)
                sprintf (args, "(%d)", dumpcommands_start[i].nargs);
            else
                args [0] = 0;
            Output("%s%s\t%s", dumpcommands_start[i].name, args,
                   dumpcommands_start[i].desc);
        }
    }
    else if (!vn || !*vn)
    {
        Output("----=====****** A summary of HaRET commands: ******=====----");
        Output("Notations used below:");
        Output("  [A|B] denotes either A or B");
        Output("  <ABC> denotes a mandatory argument");
        Output("  Any command name can be shortened to minimal unambiguous length,");
        Output("  e.g. you can use 'p' for 'priint' but not 'vd' for 'vdump'");
        for (int i = 0; i < commands_count; i++)
            if (commands_start[i].desc)
                Output("%s", commands_start[i].desc);
        Output("QUIT");
        Output("  Quit the remote session.");
    }
    else
        Output("No help on this topic available");
}
REG_CMD(0, "H|ELP", cmd_help,
        "HELP [VARS|DUMP]\n"
        "  Display a description of either commands, variables or dumpers.")

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

  for (ScriptLine = 1; ; ScriptLine++)
  {
    char str [200];
    if (!fgets (str, sizeof (str), f))
      break;

    char *x = strchr (str, 0);
    while ((x [-1] == '\n') || (x [-1] == '\r'))
      *(--x) = 0;

    scrInterpret (str, ScriptLine);
  }

  fclose (f);
}
