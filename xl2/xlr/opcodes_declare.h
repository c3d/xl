#define INFIX(t1, symbol, t2, name, code)       \
struct name : Native                            \
{                                               \
    name(): Native(NULL) {}                     \
    Tree *Run(Scope *scope) { code; }           \
};

#define PARM(symbol, type)      type##_t symbol = type##_arg(scope, argc++);

#define PREFIX(symbol, parms, name, code)                       \
struct name : Native                                            \
{                                                               \
    name(): Native(NULL) {}                                     \
    Tree *Run(Scope *scope) { uint argc=0; parms; code; }       \
};
#define POSTFIX(parms, symbol, name, code)                      \
struct name : Native                                            \
{                                                               \
    name(): Native(NULL) {}                                     \
    Tree *Run(Scope *scope) { uint argc=0; parms; code; }       \
};
#define NAME(symbol, name)
#define TYPE(symbol, name)

