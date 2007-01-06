/*
    Scripting language interface
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _SCRIPT_H
#define _SCRIPT_H

#include "xtypes.h" // uint

// Interpret one line of scripting language; returns false on QUIT
bool scrInterpret (const char *str, uint lineno);
// Execute the script from given file
extern void scrExecute (const char *scrfn, bool complain = true);
// Parse the next part of the string as an expression
bool get_expression(const char **s, uint32 *v, int priority = 0, int flags = 0);
// Parse the next string as a literal token
int get_token(const char **s, char *storage, int storesize, int for_expr=0);
// The current line of the script being parsed
extern uint ScriptLine;
// Maximum command line supported
static const int MAX_CMDLEN = 200;


/****************************************************************
 * Macros to declare a new command.
 ****************************************************************/

// Registration of script commands
#define REG_CMD(Pred, Name, Func, Desc)         \
    REG_CMD_ALT(Pred, Name, Func, , Desc)

#define REG_CMD_ALT(Pred, Name, Func, Alt, Desc)                        \
    __REG_CMD(Func ##Alt, Pred, 0, Name, Desc, cmdFunc, {0}, 0, Func)

// Registration of script dump commands
#define REG_DUMP(Pred, Name, Func, Desc)      \
    __REG_CMD(Func, Pred, 0, Name, Desc, cmdDump, {0}, 0, Func)

// Registration of variables
#define REG_VAR_STR(Pred, Name, Var, Desc)                      \
    __REG_CMD(Var, Pred, 0, Name, Desc, varString, { (uint32*)&Var } )

#define REG_VAR_INT(Pred, Name, Var, Desc)              \
    __REG_CMD(Var, Pred, 0, Name, Desc, varInteger, { &Var })

#define REG_VAR_INTLIST(Pred, Name, Var, ArgCount, Desc)        \
    __REG_CMD(Var, Pred, 0, Name, Desc, varIntList, { Var }, ArgCount)

#define REG_VAR_BITSET(Pred, Name, Var, ArgCount, Desc)         \
    __REG_CMD(Var, Pred, 0, Name, Desc, varBitSet, { Var }, ArgCount)

#define REG_VAR_ROFUNC(Pred, Name, Func, ArgCount, Desc)                \
    __REG_CMD(Func, Pred, 0, Name, Desc, varROFunc, { (uint32*)&Func }, ArgCount)

#define REG_VAR_RWFUNC(Pred, Name, Func, ArgCount, Desc)                \
    __REG_CMD(Func, Pred, 0, Name, Desc, varRWFunc, { (uint32*)&Func }, ArgCount)


/****************************************************************
 * Internals to declaring commands
 ****************************************************************/

#define __REG_CMD(Decl, Vals...)                        \
struct haret_cmd_s Ref ##Decl                           \
    __attribute__ ((__section__ (".data.cmds")))        \
    = { Vals };

// Command types (HaRET scripting has very loose type checking anyway...)
enum cmdType
{
  cmdDump,
  cmdFunc,
  varInteger,
  varString,
  varBitSet,
  varIntList,
  varROFunc,
  varRWFunc
};

// Structure to hold commands
struct haret_cmd_s {
  // Predicate function to determine if this command is available.
  int (*testAvail)();
  // Is this command active.
  int isAvail;
  // Variable name
  const char *name;
  // Variable description
  const char *desc;
  // Command type
  cmdType type;

  /*
   * Fields for variables
   */

  // The pointer to variable value
  union
  {
    uint32 *ival;
    char **sval;
    uint32 *bsval;
    uint32 (*fval) (bool setval, uint32 *args, uint32 val);
  };
  // A optional value size (for bitset in bits, for funcs number of args,
  // for string whether free is necessary, for others no meaning)
  uint val_size;

  /*
   * Fields for normal commands
   */
  void (*func)(const char *cmd, const char *args);
};

void setupCommands();

#endif /* _SCRIPT_H */
