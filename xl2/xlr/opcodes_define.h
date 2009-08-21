#define INFIX(t1, symbol, t2, name, code)                               \
    do                                                                  \
    {                                                                   \
        Infix *xdecl = new Infix(":", new Name("x"), new Name(#t1));    \
        Infix *ydecl = new Infix(":", new Name("y"), new Name(#t2));    \
        Infix *from = new Infix(symbol, xdecl, ydecl);                  \
        Scope *to = new Scope();                                        \
        to->parameterCount = 2;                                         \
        to->values.resize(2);                                           \
        to->next = new name();                                          \
        c->EnterRewrite(from, to);                                      \
    } while(0);

#define PARM(symbol, type)                                      \
        Infix *symbol##_decl = new Infix(":",                   \
                                         new Name(#symbol),     \
                                         new Name(#type));      \
        parameters = AddParameter(parameters, symbol##_decl);

#define PREFIX(symbol, parms, name, code)                               \
    do                                                                  \
    {                                                                   \
        Tree *parameters = NULL;                                        \
        parms;                                                          \
        Prefix *from = new Prefix(new Name(symbol), parameters);        \
        Scope *to = new Scope();                                        \
        to->parameterCount = 1;                                         \
        to->values.resize(1);                                           \
        to->next = new name();                                          \
        c->EnterRewrite(from, to);                                      \
    } while(0);

#define POSTFIX(t1, symbol, name, code)                                 \
    do                                                                  \
    {                                                                   \
        Tree *parameters = NULL;                                        \
        parms;                                                          \
        Prefix *from = new Postfix(parameters, new Name(symbol));       \
        Scope *to = new Scope();                                        \
        to->parameterCount = 1;                                         \
        to->values.resize(1);                                           \
        to->next = new name();                                          \
        c->EnterRewrite(from, to);                                      \
    } while(0);

#define NAME(symbol, name)                      \
    do                                          \
    {                                           \
        name *value = new name(#symbol);        \
        c->EnterName(#symbol, value);           \
        symbol##_name = value;                  \
    } while (0);

#define TYPE(symbol, name)                      \
    do                                          \
    {                                           \
        name *typechecker = new name();         \
        c->EnterName(symbol, typechecker);      \
    } while(0);
