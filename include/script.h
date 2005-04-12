/*
    Scripting language interface
    Copyright (C) 2003 Andrew Zabolotny

    For conditions of use see file COPYING
*/

#ifndef _SCRIPT_H
#define _SCRIPT_H

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
  // for others no meaning)
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

// Interpret one line of scripting language; returns false on QUIT
bool scrInterpret (const char *str, uint lineno);
// Execute the script from given file
extern void scrExecute (const char *scrfn, bool complain = true);
// Listen for a connection on given port and execute commands
void scrListen (int port);

#endif /* _SCRIPT_H */
