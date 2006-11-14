/*
    Scripting language interface
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _SCRIPT_H
#define _SCRIPT_H

// Interpret one line of scripting language; returns false on QUIT
bool scrInterpret (const char *str, uint lineno);
// Execute the script from given file
extern void scrExecute (const char *scrfn, bool complain = true);
// Parse the next part of the string as an expression
bool get_expression(const char **s, uint32 *v, int priority = 0, int flags = 0);
// Parse the next string as a literal token
char *get_token(const char **s);
// The current line of the script being parsed
extern uint ScriptLine;


/****************************************************************
 * Macros to declare a new command.
 ****************************************************************/

// Registration of script commands
#define REG_CMD(Pred, Name, Func, Desc)         \
    __REG_CMD(Pred, Name, Func, Func, Desc)

#define REG_CMD_ALT(Pred, Name, Func, Alt, Desc)        \
    __REG_CMD(Pred, Name, Func, Func ##Alt, Desc)

// Registration of variables
#define REG_VAR_STR(Pred, Name, Var, Desc)                      \
    __REG_VAR(Var, Name, Desc, varString, { (uint32*)&Var } )

#define REG_VAR_INT(Pred, Name, Var, Desc)              \
    __REG_VAR(Var, Name, Desc, varInteger, { &Var })

#define REG_VAR_BITSET(Pred, Name, Var, ArgCount, Desc)         \
    __REG_VAR(Var, Name, Desc, varBitSet, { Var }, ArgCount)

#define REG_VAR_ROFUNC(Pred, Name, Func, ArgCount, Desc)                \
    __REG_VAR(Func, Name, Desc, varROFunc, { (uint32*)&Func }, ArgCount)

#define REG_VAR_RWFUNC(Pred, Name, Func, ArgCount, Desc)                \
    __REG_VAR(Func, Name, Desc, varRWFunc, { (uint32*)&Func }, ArgCount)

// Registration of script dump commands
#define REG_DUMP(Pred, Name, Func, ArgCount, Desc)      \
struct hwDumper Ref ##Func                              \
    __attribute__ ((__section__ (".rdata.dumpcmds")))   \
    = { Name, Desc, ArgCount, Func };


/****************************************************************
 * Internals to declaring commands
 ****************************************************************/

#define __REG_CMD(Pred, Name, Func, Decl, Desc)         \
struct haret_cmd_s Ref ##Decl                           \
    __attribute__ ((__section__ (".rdata.cmds")))       \
    = { Name, Desc, Func };

#define __REG_VAR(Decl, Vals...)                        \
struct varDescriptor Ref ##Decl                         \
    __attribute__ ((__section__ (".data.vars")))        \
    = { Vals };

// Variable types (HaRET scripting has very loose type checking anyway...)
enum varType
{
  varInteger,
  varString,
  varBitSet,
  varROFunc,
  varRWFunc
};

// The list of variables and their handlers
struct varDescriptor
{
  // Variable name
  const char *name;
  // Variable description
  const char *desc;
  // Variable type
  varType type;
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

  void (*notify_set)(void);
};

// The structure to describe a hardware dumper
struct hwDumper
{
  // Hardware name
  const char *name;
  // Hardware description
  const char *desc;
  // Number of arguments
  int nargs;
  // The function that does the dump
  bool (*dump) (void (*out) (void *data, const char *, ...),
                void *data, uint32 *args);
};

// Structure to hold commands
struct haret_cmd_s {
    const char *name, *desc;
    void (*func)(const char *cmd, const char *args);
};

#endif /* _SCRIPT_H */
