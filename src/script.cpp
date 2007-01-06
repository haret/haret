/*
    Poor Man's Hardware Reverse Engineering Tool script language interpreter
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#include <windows.h> // CreateThread

#include <stdio.h> // fopen, FILE
#include <ctype.h> // isspace, toupper
#include <stdarg.h> // va_list
#include <string.h> // strchr, memcpy, memset
#include <stdlib.h> // free

#include "xtypes.h"
#include "cbitmap.h" // TEST/SET/CLEARBIT
#include "output.h" // Output, fnprepare
#include "script.h"

static const char *quotes = "\"'";
// Currently processed line (for error display)
uint ScriptLine;

// Symbols added by linker.
extern "C" {
    extern haret_cmd_s commands_start[];
    extern haret_cmd_s commands_end;
}
#define commands_count (&commands_end - commands_start)

static haret_cmd_s *UserVars = NULL;
static int UserVarsCount = 0;

// Initialize builtin commands and variables.
void
setupCommands()
{
    for (int i = 0; i < commands_count; i++) {
        haret_cmd_s *x = &commands_start[i];
        if (x->testAvail) {
            Output("Testing for command %s", x->name);
            int ret = x->testAvail();
            if (!ret) {
                Output("Not registering command %s", x->name);
                continue;
            }
            Output("Registering command %s", x->name);
        }
        x->isAvail = 1;
    }
}

int
get_token(const char **s, char *storage, int storesize, int for_expr)
{
    const char *x = *s;

    // Skip spaces at the beginning
    while (*x && isspace (*x))
        x++;

    // If at the end of string, return empty token
    if (!*x) {
        storage[0] = 0;
        return -1;
    }

    char quote = 0;
    if (strchr(quotes, *x))
        quote = *x++;

    const char *e = x;
    if (quote)
        while (*e && (*e != quote))
            e++;
    else if (for_expr)
        while (*e && isalnum (*e))
            e++;
    else
        while (*e && !isspace(*e))
            e++;

    if (e >= x + storesize)
        e = x + storesize - 1;
    memcpy (storage, x, e - x);
    storage [e - x] = 0;

    if (quote && *e)
        e++;
    *s = e;

    return 0;
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

static bool get_args (const char **s, const char *keyw, uint32 *args,
                      uint count);

static inline int isVar(haret_cmd_s *cmd) {
    return cmd->type >= varInteger;
}

static haret_cmd_s *
FindVar(const char *vn, haret_cmd_s *Vars, int VarCount)
{
    for (int i = 0; i < commands_count; i++) {
        haret_cmd_s *var = &commands_start[i];
        if (var->isAvail && isVar(var) && !strcasecmp(vn, var->name))
            return var;
    }
    return NULL;
}

static bool GetVar (const char *vn, const char **s, uint32 *v,
                    haret_cmd_s *Vars, int VarCount)
{
  haret_cmd_s *var = FindVar (vn, Vars, VarCount);
  if (!var)
    return false;

  switch (var->type)
  {
    default:
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
        Output(C_ERROR "line %d: Index out of range (0..%d)",
                  ScriptLine, var->val_size);
        return false;
      }
      *v = TESTBIT(var->bsval, *v);
      break;
    case varIntList:
      if (!get_args (s, vn, v, 1))
        return false;
      if (*v > var->val_size || *v >= var->bsval[0])
      {
        Output(C_ERROR "line %d: Index out of range (0..%d)",
                  ScriptLine, var->bsval[0]);
        return false;
      }
      *v = var->bsval[*v];
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

static haret_cmd_s *
NewVar (char *vn, cmdType vt)
{
  haret_cmd_s *ouv = UserVars;
  UserVars = (haret_cmd_s *)
      realloc(UserVars, sizeof(UserVars[0]) * (UserVarsCount + 1));
  if (UserVars != ouv)
  {
    // Since we reallocated the stuff, we have to fix the ival pointers as well
    for (int i = 0; i < UserVarsCount; i++)
      if (UserVars [i].type == varInteger)
        UserVars [i].ival = &UserVars [i].val_size;
  }

  haret_cmd_s *nv = UserVars + UserVarsCount;
  memset (nv, 0, sizeof (*nv));
  nv->name = _strdup(vn);
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
  char store[MAX_CMDLEN];
  get_token(s, store, sizeof(store), 1);
  char *x = store;

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

      case '!':
        (*s)++;
        if (!get_expression (s, v, 3, flags & ~PAREN_EAT))
          return false;
        *v = !*v;
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
        Output(C_ERROR "line %d: Unexpected input '%s'", ScriptLine, *s);
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
        Output(C_ERROR "line %d: Expected a number, got %s", ScriptLine, x);
        return false;
      }
    }
    // Look through variables
    else if (!GetVar (x, s, v, commands_start, commands_count)
          && !GetVar (x, s, v, UserVars, UserVarsCount))
    {
      Output(C_ERROR "line %d: Unknown variable '%s' in expression",
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
          Output(C_ERROR "line %d: Unexpected ')'", ScriptLine);
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
    Output(C_ERROR "line %d: No closing ')'", ScriptLine);
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
    Output(C_ERROR "line %d: %s(%d args) expected", ScriptLine, keyw, count);
    return false;
  }

  (*s)++;
  while (count--)
  {
    if (!get_expression (s, args, 0, count ? 0 : PAREN_EXPECT | PAREN_EAT))
    {
error:
      Output(C_ERROR "line %d: not enough arguments to function %s"
             , ScriptLine, keyw);
      return false;
    }

    if (!count)
      break;

    if (peek_char (s) != ',')
      goto error;

    (*s)++;
    args++;
  }

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

bool scrInterpret (const char *str, uint lineno)
{
    ScriptLine = lineno;

    const char *x = str;
    while (*x && isspace(*x))
        x++;
    if (*x == '#' || !*x)
        return true;

    char tok[MAX_CMDLEN];
    get_token(&x, tok, sizeof(tok), 1);

    // Okay, now see what keyword is this :)
    for (int i = 0; i < commands_count; i++) {
        haret_cmd_s *hc = &commands_start[i];
        if (hc->isAvail && hc->type == cmdFunc && IsToken(tok, hc->name)) {
            hc->func(tok, x);
            return true;
        }
    }

    if (IsToken(tok, "Q|UIT"))
        return false;

    Output(C_ERROR "Unknown keyword: `%s'", tok);
    return true;
}

class fileredir : public outputfn {
    FILE *f;
    outputfn *old;
public:
    int init(const char *vn) {
        char fn [200];
        fnprepare(vn, fn, sizeof (fn));

        f = fopen(fn, "wb");
        if (!f) {
            Output("line %d: Cannot open file `%s' for writing", ScriptLine, fn);
            return -1;
        }
        old = setOutputFn(this);
        return 0;
    }
    void done() {
        setOutputFn(old);
        fclose(f);
    }
    void sendMessage(const char *msg, int len) {
        fwrite(msg, len, 1, f);
    }
};

static void
cmd_dump(const char *cmd, const char *args)
{
    char vn[MAX_CMDLEN];
    if (get_token(&args, vn, sizeof(vn), 1)) {
        Output("line %d: Dumper name expected", ScriptLine);
        return;
    }

    haret_cmd_s *hwd = NULL;
    for (int i = 0; i < commands_count; i++) {
        haret_cmd_s *hd = &commands_start[i];
        if (hd->isAvail && hd->type == cmdDump && !strcasecmp(vn, hd->name)) {
            hwd = hd;
            break;
        }
    }
    if (!hwd) {
        Output("line %d: No dumper %s available, see HELP DUMP for a list"
               , ScriptLine, vn);
        return;
    }

    hwd->func(vn, args);
}
REG_CMD(0, "D|UMP", cmd_dump,
        "DUMP <hardware>[(args...)]\n"
        "  Dump the state of given hardware.\n"
        "  Use HELP DUMP to see available dumpers.")

static void
redir(const char *args)
{
    char vn[MAX_CMDLEN];
    if (get_token(&args, vn, sizeof(vn))) {
        Output(C_ERROR "line %d: file name expected", ScriptLine);
        return;
    }

    fileredir redir;
    int ret = redir.init(vn);
    if (ret)
        return;
    scrInterpret(args, ScriptLine);
    redir.done();
}

static void
bgRun(char *args)
{
    prepThread();
    redir(args);
    free(args);
}

static void
cmd_redir(const char *cmd, const char *args)
{
    if (toupper(cmd[0]) == 'B')
        // Run in background thread.
        CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)bgRun,
                     (LPVOID)_strdup(args), 0, NULL);
    else
        redir(args);
}
REG_CMD(0, "REDIR", cmd_redir,
        "REDIR <filename> <command>\n"
        "  Run <command> and send it's output to <file>")
REG_CMD_ALT(0, "BG", cmd_redir, bg,
            "BG <filename> <command>\n"
            "  Run <command> in a background thread - store output in <file>")

static void
cmd_set(const char *cmd, const char *x)
{
    char vn[MAX_CMDLEN];
    if (get_token(&x, vn, sizeof(vn))) {
        Output(C_ERROR "line %d: Expected <varname>", ScriptLine);
        return;
    }

    haret_cmd_s *var = FindVar (vn, commands_start, commands_count);
    if (!var)
        var = FindVar (vn, UserVars, UserVarsCount);
    if (!var)
        var = NewVar (vn, varInteger);

    switch (var->type) {
    case varInteger:
        if (!get_expression (&x, var->ival))
        {
            Output(C_ERROR "line %d: Expected numeric <value>", ScriptLine);
            return;
        }
        break;
    case varString:
        // If val_size is zero, it means a const char* in .text segment
        if (var->val_size)
            free(*var->sval);
        get_token(&x, vn, sizeof(vn));
        *var->sval = _strdup(vn);
        var->val_size = 1;
        break;
    case varBitSet:
    {
        uint32 idx, val;
        if (!get_expression (&x, &idx)
            || !get_expression (&x, &val))
        {
            Output(C_ERROR "line %d: Expected <index> <value>", ScriptLine);
            return;
        }
        if (idx > var->val_size)
        {
            Output(C_ERROR "line %d: Index out of range (0..%d)",
                      ScriptLine, var->val_size);
            return;
        }
        if (val)
            SETBIT(var->bsval, idx);
        else
            CLEARBIT(var->bsval, idx);
        break;
    }
    case varIntList:
    {
        uint32 idx=1;
        while (idx < var->val_size && get_expression(&x, &var->bsval[idx]))
            idx++;
        var->bsval[0] = idx;
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
            Output(C_ERROR "line %d: Expected <value>", ScriptLine);
            return;
        }
        var->fval (true, args, val);
        break;
    }
    default:
        Output(C_ERROR "line %d: `%s' is a read-only variable", ScriptLine,
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
    char vn[MAX_CMDLEN];
    get_token(&x, vn, sizeof(vn));

    if (!strcasecmp(vn, "VARS")) {
        char type [9];
        char args [10];

        Output("Name\tType\tDescription");
        Output("-----------------------------------------------------------");
        for (int i = 0; i < commands_count; i++) {
            haret_cmd_s *var = &commands_start[i];
            if (!var->isAvail || !isVar(var))
                continue;

            args [0] = 0;
            type [0] = 0;
            switch (var->type)
            {
            default:
            case varInteger:
                strcpy (type, "int");
                break;
            case varString:
                strcpy (type, "string");
                break;
            case varBitSet:
                strcpy (type, "bitset");
                break;
            case varIntList:
                strcpy (type, "int list");
                break;
            case varROFunc:
                strcpy (type, "ro func");
                // fallback
            case varRWFunc:
                if (!type[0])
                    strcpy(type, "rw func");
                if (var->val_size)
                    sprintf(args, "(%d)", var->val_size);
                break;
            }
            Output("%s%s\t%s\t%s", var->name, args, type, var->desc);
        }
    }
    else if (!strcasecmp (vn, "DUMP"))
    {
        for (int i = 0; i < commands_count; i++) {
            haret_cmd_s *hc = &commands_start[i];
            if (hc->isAvail && hc->type == cmdDump && hc->desc)
                Output("%s", hc->desc);
        }
    }
    else if (!vn[0])
    {
        Output("----=====****** A summary of HaRET commands: ******=====----");
        Output("Notations used below:");
        Output("  [A|B] denotes either A or B");
        Output("  <ABC> denotes a mandatory argument");
        Output("  Any command name can be shortened to minimal unambiguous length,");
        Output("  e.g. you can use 'p' for 'print' but not 'vd' for 'vdump'");
        for (int i = 0; i < commands_count; i++) {
            haret_cmd_s *hc = &commands_start[i];
            if (hc->isAvail && hc->type == cmdFunc && hc->desc)
                Output("%s", hc->desc);
        }
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
      Output(C_ERROR "Cannot open script file\n%s", fn);
    return;
  }

  for (int line = 1; ; line++)
  {
    char str[MAX_CMDLEN];
    if (!fgets (str, sizeof (str), f))
      break;

    char *x = str + strlen(str);
    while ((x [-1] == '\n') || (x [-1] == '\r'))
      *(--x) = 0;

    scrInterpret(str, line);
  }

  fclose (f);
}

static void
cmd_runscript(const char *cmd, const char *args)
{
    char vn[MAX_CMDLEN];
    if (get_token(&args, vn, sizeof(vn))) {
        Output(C_ERROR "line %d: file name expected", ScriptLine);
        return;
    }
    uint32 ignore = 0;
    get_expression(&args, &ignore);

    scrExecute(vn, !ignore);
}
REG_CMD(0, "R|UNSCRIPT", cmd_runscript,
        "RUNSCRIPT <filename> [<ignoreNotFound>]\n"
        "  Run the commands located in the specified file.\n"
        "  Set <ignoreNotFound> to 1 to suppress a file not found error.")

static void
cmd_test(const char *cmd, const char *args)
{
    uint32 val;
    if (!get_expression(&args, &val)) {
        Output(C_ERROR "line %d: expected <expr>", ScriptLine);
        return;
    }
    if (val)
        scrInterpret(args, ScriptLine);
}
REG_CMD(0, "IF", cmd_test,
        "IF <expr> <command>\n"
        "  Run <command> iff <expr> is non-zero.")
