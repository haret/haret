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
void runMemScript(const char *script);
// Parse the next part of the string as an expression
bool get_expression(const char **s, uint32 *v, int priority = 0, int flags = 0);
// Parse the next string as a literal token
int get_token(const char **s, char *storage, int storesize, int for_expr=0);
void ScriptError(const char *fmt, ...)
    __attribute__ ((format (printf, 1, 2)));
int arg_snprintf(char *buf, int len, const char *args);
// Maximum command line supported
static const int MAX_CMDLEN = 512;


/****************************************************************
 * Macros to declare a new command.
 ****************************************************************/

// Registration of script commands
#define REG_CMD(Pred, Name, Func, Desc)         \
    REG_CMD_ALT(Pred, Name, Func, , Desc)

#define REG_CMD_ALT(Pred, Name, Func, Alt, Desc)                \
    __REG_CMD(regCommand, Func ##Alt, Pred, Name, Desc, Func)

// Registration of script dump commands
#define REG_DUMP(Pred, Name, Func, Desc)                        \
    __REG_CMD(dumpCommand, Func, Pred, Name, Desc, Func)

// Registration of variables
#define REG_VAR_STR(Pred, Name, Var, Desc)              \
    __REG_CMD(stringVar, Var, Pred, Name, Desc, &Var )

#define REG_VAR_INT(Pred, Name, Var, Desc)              \
    __REG_CMD(integerVar, Var, Pred, Name, Desc, &Var)

#define REG_VAR_INTLIST(Pred, Name, ArgCount, Var, Desc)       \
    __REG_CMD(intListVar, Var, Pred, Name, Desc, ArgCount, Var, ARRAY_SIZE(Var))

#define REG_VAR_BITSET(Pred, Name, Var, ArgCount, Desc)         \
    __REG_CMD(bitsetVar, Var, Pred, Name, Desc, Var, ArgCount)

#define REG_VAR_ROFUNC(Pred, Name, Func, ArgCount, Desc)                \
    __REG_CMD(rofuncVar, Func, Pred, Name, Desc, Func, ArgCount)

#define REG_VAR_RWFUNC(Pred, Name, Func, ArgCount, Desc)                \
    __REG_CMD(rwfuncVar, Func, Pred, Name, Desc, Func, ArgCount)


/****************************************************************
 * Internals to declaring commands
 ****************************************************************/

#define __REG_CMD(Type, Decl, Vals...)          \
    __REG_VAR(Type, Ref ##Decl, Vals)
#define __REG_VAR(Type, Decl, Vals...)                          \
class Type Decl (Vals);                                         \
class commandBase *Ptr ##Decl                                   \
    __attribute__ ((__section__ (".rdata.cmds"))) = & Decl;

// Structure to hold commands
class commandBase {
public:
    typedef int (*predFunc)();
    commandBase(predFunc ta, const char *n, const char *d)
        : testAvail(ta), isAvail(0), name(n), desc(d) { }
    virtual ~commandBase() { }
    // Predicate function to determine if this command/variable is available.
    predFunc testAvail;
    // Is this command/variable active.
    int isAvail;
    // Command/variable name
    const char *name;
    // Command/variable description
    const char *desc;
};

class regCommand : public commandBase {
public:
    typedef void (*cmdfunc)(const char *cmd, const char *args);
    regCommand(predFunc ta, const char *n, const char *d, cmdfunc f)
        : commandBase(ta, n, d), func(f) { }
    cmdfunc func;
};

class dumpCommand : public commandBase {
public:
    typedef regCommand::cmdfunc cmdfunc;
    dumpCommand(predFunc ta, const char *n, const char *d, cmdfunc f)
        : commandBase(ta, n, d), func(f) { }
    cmdfunc func;
};

class variableBase : public commandBase {
public:
    variableBase(predFunc ta, const char *n, const char *d)
        : commandBase(ta, n, d) { }
    virtual bool getVar(const char **args, uint32 *v);
    virtual void setVar(const char *args);
    virtual void showVar(const char *args);
    virtual void clearVar(const char *args);
    virtual variableBase *newVar();
    static const int MAXTYPELEN = 16;
    virtual void fillVarType(char *buf) { }
};

class stringVar : public variableBase {
public:
    stringVar(predFunc ta, const char *n, const char *d, char **v)
        : variableBase(ta, n, d), data(v), isDynamic(0) { }
    bool getVar(const char **args, uint32 *v);
    void setVar(const char *args);
    void showVar(const char *args);
    void fillVarType(char *buf);
    char **data;
    int isDynamic;
};

class integerVar : public variableBase {
public:
    integerVar(predFunc ta, const char *n, const char *d, uint32 *v)
        : variableBase(ta, n, d), data(v), dynstorage(0) { }
    bool getVar(const char **args, uint32 *v);
    void setVar(const char *args);
    void fillVarType(char *buf);
    uint32 *data;
    uint32 dynstorage;
};

class listVarBase : public variableBase {
public:
    listVarBase(predFunc ta, const char *n, const char *d, uint32 *c
                , void *v, uint ds, uint max)
        : variableBase(ta, n, d)
        , count(c), data(v), datasize(ds), maxavail(max) { }
    bool getVar(const char **args, uint32 *v);
    void setVar(const char *args);
    void clearVar(const char *args);
    virtual bool getVarItem(void *p, const char **args, uint32 *v);
    virtual bool setVarItem(void *p, const char *args);
    uint32 *count;
    void *data;
    uint datasize;
    uint maxavail;
};

class intListVar : public listVarBase {
public:
    intListVar(predFunc ta, const char *n, const char *d, uint32 *c
               , uint32 *v, uint max)
        : listVarBase(ta, n, d, c, (void*)v, sizeof(uint32), max) { }
    bool getVarItem(void *p, const char **args, uint32 *v);
    bool setVarItem(void *p, const char *args);
    void showVar(const char *args);
    void fillVarType(char *buf);
};

class bitsetVar : public variableBase {
public:
    bitsetVar(predFunc ta, const char *n, const char *d, uint32 *v, uint max)
        : variableBase(ta, n, d), data(v), maxavail(max) { }
    bool getVar(const char **args, uint32 *v);
    void setVar(const char *args);
    void fillVarType(char *buf);
    uint32 *data;
    uint maxavail;
};

class rofuncVar : public variableBase {
public:
    typedef uint32 (*varfunc_t)(bool setval, uint32 *args, uint32 val);
    rofuncVar(predFunc ta, const char *n, const char *d, varfunc_t f, int na)
        : variableBase(ta, n, d), func(f), numargs(na) { }
    bool getVar(const char **args, uint32 *v);
    void fillVarType(char *buf);
    varfunc_t func;
    int numargs;
};

class rwfuncVar : public rofuncVar {
public:
    rwfuncVar(predFunc ta, const char *n, const char *d, varfunc_t f, int na)
        : rofuncVar(ta, n, d, f, na) { }
    void setVar(const char *args);
    void fillVarType(char *buf);
};

void setupCommands();
variableBase *FindVar(const char *vn);
void SetVar(const char *name, const char *val);

#endif /* _SCRIPT_H */
